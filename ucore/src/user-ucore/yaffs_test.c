#include <ulib.h>
#include <stdio.h>
#include <string.h>
#include <stat.h>
#include <dirent.h>
#include <file.h>
#include <dir.h>
#include <unistd.h>

#define printf(...)                 fprintf(1, __VA_ARGS__)

static void safe_chdir(const char *path)
{
	int ret = chdir(path);
	assert(ret == 0);
}

static void safe_getcwd(round)
{
	static char buffer[FS_MAX_FPATH_LEN + 1];
	int ret = getcwd(buffer, sizeof(buffer));
	assert(ret == 0);
	printf("%d: current: %s\n", round, buffer);
}

void changedir(const char *path)
{
	static int round = 0;
	printf("------------------------\n");
	safe_chdir(path);
	safe_getcwd(round);
	round++;
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

int main(int argc, char *argv[])
{
	int i;
	cprintf("yaffs_test!!.\n");
	for (i = 0; i < argc; i++)
		cprintf("  ARG %d: %s\n", i, argv[i]);

	char *nanddisk = "disk1:/";
	if (argc > 1)
		nanddisk = argv[1];

	changedir(nanddisk);

	int fd1 = open("testfile", O_CREAT | O_RDWR | O_TRUNC);
	assert(fd1);

	char *buf = (char *)malloc(4096);
	for (i = 0; i < 4096; i++)
		buf[i] = 'A' + i % 26;

	safe_write(fd1, buf, 2789);

	close(fd1);

	fd1 = open("testfile", O_RDWR);
	safe_seek(fd1, 10, LSEEK_SET);
	safe_read(fd1, buf, 16);
	buf[16] = 0;
	printf("read: %s\n", buf);

	free(buf);
	return 0;
}
