#include <ulib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stat.h>
#include <file.h>
#include <dir.h>
#include <unistd.h>

#define printf(...)                 fprintf(1, __VA_ARGS__)

static int safe_open(const char *path, int open_flags)
{
	int fd = open(path, open_flags);
	assert(fd >= 0);
	return fd;
}

static struct stat *safe_fstat(int fd)
{
	static struct stat __stat, *stat = &__stat;
	int ret = fstat(fd, stat);
	assert(ret == 0);
	return stat;
}

static void safe_read(int fd, void *data, size_t len)
{
	int ret = read(fd, data, len);
	assert(ret == len);
}

static void safe_write(int fd, void *data, size_t len)
{
	int ret = write(fd, data, len);
	assert(ret == len);
}

static void safe_seek(int fd, off_t pos, int whence)
{
	int ret = seek(fd, pos, whence);
	assert(ret == 0);
}

static unsigned char buffer[4096];

int main(void)
{
	int fd = safe_open("/testdir/test/testfile", O_RDWR | O_TRUNC);
	struct stat *stat = safe_fstat(fd);
	assert(stat->st_size == 0 && stat->st_blocks == 0);

	safe_seek(fd, sizeof(buffer) * 1024 * 2, LSEEK_SET);

	int i;
	for (i = 0; i < sizeof(buffer); i++) {
		buffer[i] = i;
	}
	safe_write(fd, buffer, sizeof(buffer));
	memset(buffer, 0, sizeof(buffer));

	safe_seek(fd, -sizeof(buffer), LSEEK_END);
	safe_read(fd, buffer, sizeof(buffer));

	for (i = 0; i < sizeof(buffer); i++) {
		assert(buffer[i] == (unsigned char)i);
	}

	fd = safe_open("/testdir/test/testfile", O_RDWR | O_TRUNC);
	stat = safe_fstat(fd);
	assert(stat->st_size == 0 && stat->st_blocks == 0);
	printf("sfs_filetest2 pass.\n");
	return 0;
}
