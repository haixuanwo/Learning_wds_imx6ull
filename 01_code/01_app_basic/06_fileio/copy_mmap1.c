#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/*
 * ./copy 1.txt 2.txt
 * argc    = 3
 * argv[0] = "./copy"
 * argv[1] = "1.txt"
 * argv[2] = "2.txt"
 */

/* 错误处理宏 */
#define ERROR_EXIT(msg) do { \
    fprintf(stderr, "Error: %s: %s\n", msg, strerror(errno)); \
    exit(EXIT_FAILURE); \
} while(0)

/* 安全关闭文件描述符 */
static inline void safe_close(int *fd)
{
    if (*fd >= 0) {
        close(*fd);
        *fd = -1;
    }
}

/* 安全取消内存映射 */
static inline void safe_munmap(void **addr, size_t len)
{
    if (*addr != NULL && *addr != MAP_FAILED) {
        munmap(*addr, len);
        *addr = NULL;
    }
}

/* 清理资源的结构体 */
struct cleanup_resources {
    int fd_old;
    int fd_new;
    void *mapped_addr;
    size_t mapped_size;
};

/* 初始化清理资源 */
static void init_cleanup(struct cleanup_resources *res)
{
    if (NULL == res)
    {
        return;
    }

    res->fd_old = -1;
    res->fd_new = -1;
    res->mapped_addr = NULL;
    res->mapped_size = 0;
}

/* 清理所有资源 */
static void cleanup(struct cleanup_resources *res)
{
    if (NULL == res)
    {
        return;
    }

    safe_munmap(&res->mapped_addr, res->mapped_size);
    safe_close(&res->fd_old);
    safe_close(&res->fd_new);
}

/* 检查文件是否为常规文件 */
static int is_regular_file(int fd)
{
    struct stat st;
    if (fstat(fd, &st) == -1) {
        return 0;
    }
    return S_ISREG(st.st_mode);
}

/* 处理大文件的分块复制 */
static int copy_large_file(int fd_src, int fd_dst, off_t file_size)
{
    const size_t CHUNK_SIZE = 64 * 1024 * 1024; // 64MB
    char *buf = NULL;
    off_t offset = 0;
    int ret = 0;

    buf = malloc(CHUNK_SIZE);
    if (!buf) {
        ERROR_EXIT("Failed to allocate buffer");
    }

    while (offset < file_size) {
        size_t chunk = (file_size - offset > CHUNK_SIZE) ?
                      CHUNK_SIZE : (size_t)(file_size - offset);
        ssize_t read_bytes, written_bytes;

        /* 读取数据 */
        read_bytes = pread(fd_src, buf, chunk, offset);
        if (read_bytes != (ssize_t)chunk) {
            if (read_bytes < 0) {
                fprintf(stderr, "Read error at offset %lld: %s\n",
                       (long long)offset, strerror(errno));
                ret = -1;
                break;
            }
            chunk = (size_t)read_bytes; // 处理短读
        }

        /* 写入数据 */
        written_bytes = pwrite(fd_dst, buf, chunk, offset);
        if (written_bytes != (ssize_t)chunk) {
            fprintf(stderr, "Write error at offset %lld: %s\n",
                   (long long)offset, strerror(errno));
            ret = -1;
            break;
        }

        offset += written_bytes;

        /* 显示进度（可选）*/
        if (isatty(STDOUT_FILENO)) {
            fprintf(stdout, "\rProgress: %.2f%%",
                   (double)offset * 100 / file_size);
            fflush(stdout);
        }
    }

    if (isatty(STDOUT_FILENO)) {
        fprintf(stdout, "\n");
    }

    free(buf);
    return ret;
}

/* 处理稀疏文件 - 只复制实际数据块 */
static int handle_sparse_file(int fd_src, int fd_dst, off_t file_size)
{
    char buffer[4096];
    off_t src_offset = 0;
    off_t dst_offset = 0;

    while (src_offset < file_size) {
        ssize_t read_bytes = pread(fd_src, buffer, sizeof(buffer), src_offset);
        if (read_bytes <= 0) {
            if (read_bytes < 0) {
                perror("Read error");
            }
            break;
        }

        /* 检查是否为全零块 */
        int is_zero_block = 1;
        for (ssize_t i = 0; i < read_bytes; i++) {
            if (buffer[i] != 0) {
                is_zero_block = 0;
                break;
            }
        }

        if (is_zero_block) {
            /* 对于零块，直接跳转到下一个位置 */
            dst_offset += read_bytes;
            lseek(fd_dst, read_bytes, SEEK_CUR);
        } else {
            /* 写入实际数据 */
            ssize_t written_bytes = pwrite(fd_dst, buffer, read_bytes, dst_offset);
            if (written_bytes != read_bytes) {
                fprintf(stderr, "Write error: %s\n", strerror(errno));
                return -1;
            }
            dst_offset += written_bytes;
        }

        src_offset += read_bytes;
    }

    /* 设置目标文件大小 */
    if (ftruncate(fd_dst, file_size) == -1) {
        fprintf(stderr, "Failed to truncate destination file: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    struct cleanup_resources res = {-1, -1, NULL, 0};
    struct stat statbuf;
    const off_t MMAP_THRESHOLD = 100 * 1024 * 1024; // 100MB阈值
    int use_sparse_detection = 1; // 是否启用稀疏文件检测

    /* 1. 参数验证 */
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <old-file> <new-file>\n", argv[0]);
        fprintf(stderr, "Example: %s source.txt destination.txt\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* 检查源文件是否相同 */
    if (strcmp(argv[1], argv[2]) == 0) {
        fprintf(stderr, "Error: Source and destination files are the same\n");
        return EXIT_FAILURE;
    }

    /* 2. 打开源文件 */
    res.fd_old = open(argv[1], O_RDONLY);
    if (res.fd_old == -1) {
        ERROR_EXIT("Failed to open source file");
    }

    /* 3. 获取源文件信息 */
    if (fstat(res.fd_old, &statbuf) == -1) {
        ERROR_EXIT("Failed to get file status");
    }

    /* 检查文件类型 */
    if (!S_ISREG(statbuf.st_mode)) {
        fprintf(stderr, "Warning: %s is not a regular file\n", argv[1]);
    }

    /* 4. 检查文件大小 */
    if (statbuf.st_size == 0) {
        /* 空文件，只需创建目标文件 */
        res.fd_new = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC,
                         S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (res.fd_new == -1) {
            ERROR_EXIT("Failed to create destination file");
        }
        printf("Copied empty file\n");
        cleanup(&res);
        return EXIT_SUCCESS;
    }

    /* 5. 创建目标文件 */
    res.fd_new = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (res.fd_new == -1) {
        ERROR_EXIT("Failed to create destination file");
    }

    /* 6. 根据文件大小选择合适的复制策略 */
    if (statbuf.st_size > MMAP_THRESHOLD) {
        /* 大文件：使用分块复制 */
        printf("Large file detected (%lld bytes), using chunked copy...\n",
               (long long)statbuf.st_size);
        if (copy_large_file(res.fd_old, res.fd_new, statbuf.st_size) != 0) {
            fprintf(stderr, "Failed to copy large file\n");
            cleanup(&res);
            /* 删除不完整的文件 */
            unlink(argv[2]);
            return EXIT_FAILURE;
        }
        printf("Large file copied successfully\n");
    } else if (use_sparse_detection && (statbuf.st_size > 1024 * 1024)) {
        /* 中等大小文件：检查是否为稀疏文件 */
        printf("Checking for sparse file...\n");
        if (handle_sparse_file(res.fd_old, res.fd_new, statbuf.st_size) != 0) {
            fprintf(stderr, "Failed to handle sparse file\n");
            cleanup(&res);
            unlink(argv[2]);
            return EXIT_FAILURE;
        }
        printf("File copied with sparse detection\n");
    } else {
        /* 小文件：使用内存映射 */
        printf("Small file detected, using memory mapping...\n");
        res.mapped_addr = mmap(NULL, statbuf.st_size, PROT_READ,
                              MAP_PRIVATE, res.fd_old, 0);
        if (res.mapped_addr == MAP_FAILED) {
            ERROR_EXIT("Failed to mmap source file");
        }
        res.mapped_size = statbuf.st_size;

        /* 建议内核提前读入数据 */
        madvise(res.mapped_addr, res.mapped_size, MADV_SEQUENTIAL);

        /* 写入目标文件 */
        ssize_t written = write(res.fd_new, res.mapped_addr, res.mapped_size);
        if (written != (ssize_t)res.mapped_size) {
            ERROR_EXIT("Failed to write destination file");
        }

        /* 确保数据写入磁盘 */
        if (fsync(res.fd_new) == -1) {
            fprintf(stderr, "Warning: fsync failed: %s\n", strerror(errno));
        }

        printf("File copied successfully using mmap\n");
    }

    /* 7. 设置目标文件的权限和修改时间 */
    struct timespec times[2];
    times[0] = statbuf.st_atim;  // 访问时间
    times[1] = statbuf.st_mtim;  // 修改时间

    if (futimens(res.fd_new, times) == -1) {
        fprintf(stderr, "Warning: Failed to set file timestamps: %s\n",
                strerror(errno));
    }

    /* 8. 清理资源 */
    cleanup(&res);

    printf("Copy completed successfully\n");
    return EXIT_SUCCESS;
}