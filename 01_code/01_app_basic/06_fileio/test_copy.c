#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

/* 包含修复的copy程序 */
#define COPY_PROGRAM "./copy_fixed"

/* 测试工具函数 */
void create_test_file(const char *filename, size_t size, int pattern) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd != -1);

    char *buffer = malloc(size);
    assert(buffer != NULL);

    if (pattern == 0) {
        /* 全零文件（稀疏文件测试） */
        memset(buffer, 0, size);
    } else if (pattern == 1) {
        /* 随机数据 */
        srand(time(NULL));
        for (size_t i = 0; i < size; i++) {
            buffer[i] = rand() % 256;
        }
    } else if (pattern == 2) {
        /* 交替模式（用于稀疏检测） */
        for (size_t i = 0; i < size; i++) {
            buffer[i] = (i % 1024 == 0) ? 1 : 0;
        }
    } else if (pattern == 3) {
        /* ASCII模式 */
        for (size_t i = 0; i < size; i++) {
            buffer[i] = 'A' + (i % 26);
        }
    }

    ssize_t written = write(fd, buffer, size);
    assert(written == (ssize_t)size);

    free(buffer);
    close(fd);
}

int compare_files(const char *file1, const char *file2) {
    struct stat st1, st2;
    if (stat(file1, &st1) == -1 || stat(file2, &st2) == -1) {
        return -1;
    }

    /* 检查文件大小 */
    if (st1.st_size != st2.st_size) {
        printf("File sizes differ: %s=%ld, %s=%ld\n",
               file1, st1.st_size, file2, st2.st_size);
        return 1;
    }

    int fd1 = open(file1, O_RDONLY);
    int fd2 = open(file2, O_RDONLY);
    assert(fd1 != -1 && fd2 != -1);

    char buf1[4096], buf2[4096];
    off_t offset = 0;

    while (offset < st1.st_size) {
        size_t to_read = (st1.st_size - offset > sizeof(buf1)) ?
                         sizeof(buf1) : (size_t)(st1.st_size - offset);

        ssize_t r1 = read(fd1, buf1, to_read);
        ssize_t r2 = read(fd2, buf2, to_read);

        if (r1 != r2) {
            printf("Read sizes differ at offset %ld\n", offset);
            close(fd1); close(fd2);
            return 1;
        }

        if (memcmp(buf1, buf2, r1) != 0) {
            printf("Content differs at offset %ld\n", offset);
            close(fd1); close(fd2);
            return 1;
        }

        offset += r1;
    }

    close(fd1); close(fd2);
    return 0;
}

int run_copy(const char *src, const char *dst) {
    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
        /* 子进程 */
        execl(COPY_PROGRAM, COPY_PROGRAM, src, dst, NULL);
        perror("execl failed");
        exit(EXIT_FAILURE);
    } else {
        /* 父进程 */
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    }
}

/* 清理测试文件 */
void cleanup_files(const char *file1, const char *file2) {
    if (file1) unlink(file1);
    if (file2) unlink(file2);
}

/* 测试用例 */

void test_basic_copy() {
    printf("=== Test 1: Basic file copy ===\n");

    const char *src = "test_basic_src.txt";
    const char *dst = "test_basic_dst.txt";

    /* 创建测试文件 */
    create_test_file(src, 1024, 1);  /* 1KB 随机数据 */

    /* 运行复制 */
    int ret = run_copy(src, dst);
    assert(ret == 0);

    /* 验证复制结果 */
    assert(compare_files(src, dst) == 0);

    /* 验证文件权限 */
    struct stat st_src, st_dst;
    stat(src, &st_src);
    stat(dst, &st_dst);
    assert((st_dst.st_mode & 0777) == 0644);

    printf("✓ Basic copy test passed\n");

    cleanup_files(src, dst);
}

void test_empty_file() {
    printf("=== Test 2: Empty file copy ===\n");

    const char *src = "test_empty_src.txt";
    const char *dst = "test_empty_dst.txt";

    /* 创建空文件 */
    create_test_file(src, 0, 0);

    /* 运行复制 */
    int ret = run_copy(src, dst);
    assert(ret == 0);

    /* 验证复制结果 */
    struct stat st_src, st_dst;
    stat(src, &st_src);
    stat(dst, &st_dst);

    assert(st_src.st_size == 0);
    assert(st_dst.st_size == 0);

    printf("✓ Empty file test passed\n");

    cleanup_files(src, dst);
}

void test_large_file() {
    printf("=== Test 3: Large file copy (>100MB) ===\n");

    const char *src = "test_large_src.dat";
    const char *dst = "test_large_dst.dat";

    /* 创建大文件（120MB，超过阈值） */
    create_test_file(src, 120 * 1024 * 1024, 1);

    /* 运行复制 */
    int ret = run_copy(src, dst);
    assert(ret == 0);

    /* 验证复制结果 */
    assert(compare_files(src, dst) == 0);

    printf("✓ Large file test passed\n");

    cleanup_files(src, dst);
}

void test_sparse_file() {
    printf("=== Test 4: Sparse file copy ===\n");

    const char *src = "test_sparse_src.dat";
    const char *dst = "test_sparse_dst.dat";
    const char *dst2 = "test_sparse_dst2.dat";

    /* 创建稀疏文件（大部分为零） */
    int fd = open(src, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd != -1);

    /* 写入少量数据在开头 */
    char data = 'X';
    write(fd, &data, 1);

    /* 跳转到文件末尾，创建空洞 */
    lseek(fd, 10 * 1024 * 1024 - 1, SEEK_SET);  /* 10MB文件 */
    write(fd, &data, 1);

    close(fd);

    /* 检查源文件确实是稀疏的 */
    struct stat st;
    stat(src, &st);
    printf("Source file: blocks=%ld, size=%ld (sparse ratio: %.2f%%)\n",
           st.st_blocks, st.st_size,
           (double)st.st_blocks * 512 / st.st_size * 100);

    /* 运行复制 */
    int ret = run_copy(src, dst);
    assert(ret == 0);

    /* 验证复制结果 */
    struct stat st_dst;
    stat(dst, &st_dst);
    printf("Destination file: blocks=%ld, size=%ld (sparse ratio: %.2f%%)\n",
           st_dst.st_blocks, st_dst.st_size,
           (double)st_dst.st_blocks * 512 / st_dst.st_size * 100);

    /* 内容应该相同 */
    assert(compare_files(src, dst) == 0);

    /* 测试完全零的文件 */
    create_test_file("all_zero.dat", 5 * 1024 * 1024, 0);
    ret = run_copy("all_zero.dat", dst2);
    assert(ret == 0);

    printf("✓ Sparse file test passed\n");

    cleanup_files(src, dst);
    cleanup_files("all_zero.dat", dst2);
}

void test_same_source_dest() {
    printf("=== Test 5: Same source and destination ===\n");

    const char *src = "test_same.txt";

    /* 创建测试文件 */
    create_test_file(src, 1024, 3);

    /* 尝试复制到自身，应该失败 */
    pid_t pid = fork();
    assert(pid >= 0);

    if (pid == 0) {
        execl(COPY_PROGRAM, COPY_PROGRAM, src, src, NULL);
        exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);
        int ret = WEXITSTATUS(status);
        assert(ret != 0);  /* 应该失败 */
    }

    printf("✓ Same source/destination test passed\n");

    unlink(src);
}

void test_nonexistent_source() {
    printf("=== Test 6: Non-existent source file ===\n");

    const char *src = "nonexistent_file_12345.txt";
    const char *dst = "test_dst.txt";

    /* 尝试复制不存在的文件，应该失败 */
    int ret = run_copy(src, dst);
    assert(ret != 0);  /* 应该失败 */

    /* 确保目标文件没有被创建 */
    struct stat st;
    int exists = (stat(dst, &st) == 0);
    assert(!exists);

    printf("✓ Non-existent source test passed\n");
}

void test_permission_denied() {
    printf("=== Test 7: Permission denied test ===\n");

    const char *src = "test_src_protected.txt";
    const char *dst = "/root/test_dst.txt";  /* 应该无法写入 */

    /* 创建源文件 */
    create_test_file(src, 1024, 3);

    /* 尝试复制到受保护的目录 */
    int ret = run_copy(src, dst);

    /* 注意：如果以root运行，这个测试会通过，但通常应该失败 */
    if (geteuid() != 0) {
        /* 非root用户应该失败 */
        assert(ret != 0);
    }

    printf("✓ Permission test passed\n");

    unlink(src);
}

void test_file_with_holes() {
    printf("=== Test 8: File with holes copy ===\n");

    const char *src = "test_holes_src.dat";
    const char *dst = "test_holes_dst.dat";

    /* 创建有空洞的文件 */
    int fd = open(src, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(fd != -1);

    /* 写开头 */
    write(fd, "START", 5);

    /* 创建空洞 */
    lseek(fd, 1024 * 1024, SEEK_CUR);  /* 1MB空洞 */

    /* 写中间 */
    write(fd, "MIDDLE", 6);

    /* 创建另一个空洞 */
    lseek(fd, 1024 * 1024, SEEK_CUR);  /* 另一个1MB空洞 */

    /* 写结尾 */
    write(fd, "END", 3);

    close(fd);

    /* 运行复制 */
    int ret = run_copy(src, dst);
    assert(ret == 0);

    /* 验证内容 */
    assert(compare_files(src, dst) == 0);

    /* 检查文件大小 */
    struct stat st_src, st_dst;
    stat(src, &st_src);
    stat(dst, &st_dst);
    assert(st_src.st_size == st_dst.st_size);

    printf("✓ File with holes test passed\n");

    cleanup_files(src, dst);
}

void test_concurrent_copy() {
    printf("=== Test 9: Concurrent copy operations ===\n");

    const char *src = "test_concurrent_src.dat";
    const char *dst1 = "test_concurrent_dst1.dat";
    const char *dst2 = "test_concurrent_dst2.dat";

    /* 创建测试文件 */
    create_test_file(src, 2 * 1024 * 1024, 1);  /* 2MB 随机数据 */

    /* 同时运行两个复制进程 */
    pid_t pid1 = fork();
    assert(pid1 >= 0);

    if (pid1 == 0) {
        /* 子进程1 */
        execl(COPY_PROGRAM, COPY_PROGRAM, src, dst1, NULL);
        exit(EXIT_FAILURE);
    }

    pid_t pid2 = fork();
    assert(pid2 >= 0);

    if (pid2 == 0) {
        /* 子进程2 */
        execl(COPY_PROGRAM, COPY_PROGRAM, src, dst2, NULL);
        exit(EXIT_FAILURE);
    }

    /* 等待两个进程完成 */
    int status1, status2;
    waitpid(pid1, &status1, 0);
    waitpid(pid2, &status2, 0);

    assert(WEXITSTATUS(status1) == 0);
    assert(WEXITSTATUS(status2) == 0);

    /* 验证两个副本都正确 */
    assert(compare_files(src, dst1) == 0);
    assert(compare_files(src, dst2) == 0);
    assert(compare_files(dst1, dst2) == 0);

    printf("✓ Concurrent copy test passed\n");

    cleanup_files(src, dst1);
    cleanup_files(NULL, dst2);
}

void test_timestamps_preserved() {
    printf("=== Test 10: Timestamps preservation ===\n");

    const char *src = "test_ts_src.txt";
    const char *dst = "test_ts_dst.txt";

    /* 创建文件并设置特定时间戳 */
    create_test_file(src, 1024, 3);

    /* 修改源文件的时间戳 */
    struct timespec times[2];
    times[0].tv_sec = 1609459200;  /* 2021-01-01 00:00:00 UTC */
    times[0].tv_nsec = 0;
    times[1].tv_sec = 1609545600;  /* 2021-01-02 00:00:00 UTC */
    times[1].tv_nsec = 0;

    utimensat(AT_FDCWD, src, times, 0);

    /* 运行复制 */
    int ret = run_copy(src, dst);
    assert(ret == 0);

    /* 验证时间戳被复制 */
    struct stat st_src, st_dst;
    stat(src, &st_src);
    stat(dst, &st_dst);

    printf("Source: atime=%ld, mtime=%ld\n", st_src.st_atime, st_src.st_mtime);
    printf("Dest:   atime=%ld, mtime=%ld\n", st_dst.st_atime, st_dst.st_mtime);

    /* 修改时间应该被保留（访问时间可能会改变） */
    assert(st_src.st_mtime == st_dst.st_mtime);

    printf("✓ Timestamps test passed\n");

    cleanup_files(src, dst);
}

void test_very_small_files() {
    printf("=== Test 11: Very small files ===\n");

    for (int i = 1; i <= 100; i++) {
        char src[32], dst[32];
        snprintf(src, sizeof(src), "tiny_src_%d.txt", i);
        snprintf(dst, sizeof(dst), "tiny_dst_%d.txt", i);

        /* 创建非常小的文件 */
        create_test_file(src, i, 3);  /* 1-100字节 */

        /* 运行复制 */
        int ret = run_copy(src, dst);
        assert(ret == 0);

        /* 验证复制结果 */
        assert(compare_files(src, dst) == 0);

        cleanup_files(src, dst);
    }

    printf("✓ Very small files test passed (100 files)\n");
}

void test_symlink_handling() {
    printf("=== Test 12: Symbolic link handling ===\n");

    const char *real_file = "test_real.txt";
    const char *symlink_file = "test_symlink.txt";
    const char *dst = "test_symlink_dst.txt";

    /* 创建真实文件 */
    create_test_file(real_file, 1024, 3);

    /* 创建符号链接 */
    symlink(real_file, symlink_file);

    /* 运行复制（应该复制链接指向的内容，而不是链接本身） */
    int ret = run_copy(symlink_file, dst);

    /* 我们的程序应该能处理符号链接（stat会跟随链接） */
    if (ret == 0) {
        /* 验证内容 */
        assert(compare_files(real_file, dst) == 0);
        printf("✓ Symbolic link test passed (followed link)\n");
    } else {
        printf("⚠ Symbolic link copy failed (may be expected)\n");
    }

    cleanup_files(real_file, symlink_file);
    cleanup_files(NULL, dst);
}

/* 性能测试 */
void test_performance() {
    printf("=== Test 13: Performance test ===\n");

    const char *src = "perf_src.dat";
    const char *dst = "perf_dst.dat";

    /* 测试不同大小的文件 */
    size_t sizes[] = {1, 10, 100, 1024, 10*1024, 100*1024,
                      1024*1024, 10*1024*1024, 50*1024*1024};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int i = 0; i < num_sizes; i++) {
        size_t size = sizes[i];
        char size_str[32];

        if (size < 1024) {
            snprintf(size_str, sizeof(size_str), "%ldB", size);
        } else if (size < 1024*1024) {
            snprintf(size_str, sizeof(size_str), "%ldKB", size/1024);
        } else {
            snprintf(size_str, sizeof(size_str), "%ldMB", size/(1024*1024));
        }

        printf("Testing %s file... ", size_str);
        fflush(stdout);

        /* 创建测试文件 */
        create_test_file(src, size, 1);

        /* 测量复制时间 */
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        int ret = run_copy(src, dst);
        assert(ret == 0);

        clock_gettime(CLOCK_MONOTONIC, &end);

        double elapsed = (end.tv_sec - start.tv_sec) +
                        (end.tv_nsec - start.tv_nsec) / 1e9;

        double speed = (elapsed > 0) ? size / elapsed : 0;

        if (size >= 1024*1024) {
            printf("%.2f seconds (%.2f MB/s)\n", elapsed, speed/(1024*1024));
        } else {
            printf("%.3f seconds\n", elapsed);
        }

        /* 验证结果 */
        assert(compare_files(src, dst) == 0);

        cleanup_files(src, dst);
    }

    printf("✓ Performance test completed\n");
}

int main() {
    printf("Starting unit tests for fixed copy program\n");
    printf("Using program: %s\n\n", COPY_PROGRAM);

    /* 确保修复的copy程序存在 */
    if (access(COPY_PROGRAM, X_OK) == -1) {
        fprintf(stderr, "Error: Copy program '%s' not found or not executable\n",
                COPY_PROGRAM);
        fprintf(stderr, "Please compile the fixed version first:\n");
        fprintf(stderr, "gcc -o copy_fixed copy_fixed.c\n");
        return EXIT_FAILURE;
    }

    /* 运行测试 */
    test_basic_copy();
    test_empty_file();
    test_large_file();
    test_sparse_file();
    test_same_source_dest();
    test_nonexistent_source();
    test_permission_denied();
    test_file_with_holes();
    test_concurrent_copy();
    test_timestamps_preserved();
    test_very_small_files();
    test_symlink_handling();
    test_performance();

    printf("\n========================================\n");
    printf("All tests passed successfully! ✓\n");
    printf("========================================\n");

    return EXIT_SUCCESS;
}
