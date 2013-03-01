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

static int safe_dup(int fd1)
{
	int fd2 = dup(fd1);
	assert(fd2 >= 0);
	return fd2;
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

static uint32_t buffer[1024];

void init_data(int fd, int pages)
{
	int i, j;
	uint32_t value = 0;
	for (i = 0; i < pages; i++) {
		for (j = 0; j < sizeof(buffer) / sizeof(buffer[0]); j++) {
			buffer[j] = value++;
		}
		safe_write(fd, buffer, sizeof(buffer));
	}
}

void random_test(int fd, int pages)
{
	int i, j;
	const int size = sizeof(buffer) / sizeof(buffer[0]);
	safe_seek(fd, 0, LSEEK_SET);
	for (i = 0; i < pages; i++) {
		safe_read(fd, buffer, sizeof(buffer));
		for (j = 0; j < 32; j++) {
			uint32_t value = rand() % size;
			assert(buffer[value] == i * size + value);
		}
	}
}

int main(void)
{
	int fd1 = safe_open("/testdir/test/testfile", O_RDWR | O_TRUNC);
	struct stat *stat = safe_fstat(fd1);
	assert(stat->st_size == 0 && stat->st_blocks == 0);

	const int npages = 128;
	init_data(fd1, npages);
	printf("init_data ok.\n");

	int fd2 = safe_dup(fd1);
	stat = safe_fstat(fd2);
	assert(stat->st_size == npages * sizeof(buffer));

	random_test(fd2, npages);
	printf("random_test ok.\n");

	int fd3 = safe_open("/testdir/test/testfile", O_RDWR | O_TRUNC);
	stat = safe_fstat(fd3);
	assert(stat->st_size == 0 && stat->st_blocks == 0);
	safe_seek(fd3, sizeof(buffer), LSEEK_END);
	safe_seek(fd3, 0, LSEEK_SET);
	stat = safe_fstat(fd3);
	assert(stat->st_size == sizeof(buffer));

	safe_seek(fd3, sizeof(buffer), LSEEK_END);
	stat = safe_fstat(fd3);
	assert(stat->st_size == sizeof(buffer) * 2);

	safe_seek(fd3, -sizeof(buffer), LSEEK_CUR);
	safe_read(fd3, buffer, sizeof(buffer));
	int i;
	for (i = 0; i < sizeof(buffer) / sizeof(buffer[0]); i++) {
		assert(buffer[i] == 0);
	}
	int fd4 = safe_open("/testdir/test/testfile", O_WRONLY | O_TRUNC);
	stat = safe_fstat(fd4);
	assert(stat->st_size == 0 && stat->st_blocks == 0);
	printf("sfs_filetest1 pass.\n");
	return 0;
}
