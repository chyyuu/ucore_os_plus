#include <ulib.h>
#include <stdio.h>
#include <string.h>
#include <stat.h>
#include <dirent.h>
#include <file.h>
#include <dir.h>
#include <error.h>
#include <malloc.h>
#include <unistd.h>

#define printf(...)                 fprintf(1, __VA_ARGS__)

#define BUF_SIZE 4096
int main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("usage: cp [src] [dst]\n");
		exit(0);
	}

	int fd1 = open(argv[1], O_EXCL | O_RDONLY);
	if (fd1 < 0) {
		printf("fail to open %s\n", argv[1]);
		return -E_NOENT;
	}

	int fd2 = open(argv[2], O_CREAT | O_RDWR | O_TRUNC);
	if (fd2 < 0) {
		printf("fail to open %s\n", argv[2]);
		close(fd1);
		return -E_INVAL;
	}

	char *buf = (char *)malloc(BUF_SIZE);
	if (!buf) {
		printf("out of memory\n");
		close(fd1);
		close(fd2);
		return -E_NO_MEM;
	}

	int readsize;
	while (1) {
		readsize = read(fd1, buf, BUF_SIZE);
		if (readsize <= 0)
			break;
		write(fd2, buf, readsize);
	}
	close(fd1);
	close(fd2);

	free(buf);
	return 0;
}
