/*
 * @Author: Clark
 * @Email: haixuanwoTxh@gmail.com
 * @Date: 2025-12-18 09:56:27
 * @LastEditors: Clark
 * @LastEditTime: 2025-12-18 11:29:45
 * @Description: file content
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

/*
 * ./hello_drv_test -w on
 * ./hello_drv_test -r
 */
int main(int argc, char **argv)
{
	int fd;
	char buf[1024];
	int len;

	/* 1. 判断参数 */
	if (argc < 2)
	{
		printf("Usage: %s -w <string>\n", argv[0]);
		printf("       %s -r\n", argv[0]);
		return -1;
	}

	/* 2. 打开文件 */
	fd = open("/dev/txh_led_device", O_RDWR);
	if (fd == -1)
	{
		printf("can not open file /dev/led\n");
		return -1;
	}

	/* 3. 写文件或读文件 */
    char status = 0;
	if ((0 == strcmp(argv[1], "on")))
	{
        status = 1;
	}

    printf("T --- led status = [%d]\n", status);
    write(fd, &status, sizeof(status));

	close(fd);
	return 0;
}
