#include <ulib.h>
#include <stdio.h>
#include <string.h>
#include <stat.h>
#include <dirent.h>
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

static char getmode(uint32_t st_mode)
{
	char mode = '?';
	if (S_ISREG(st_mode))
		mode = '-';
	if (S_ISDIR(st_mode))
		mode = 'd';
	if (S_ISLNK(st_mode))
		mode = 'l';
	if (S_ISCHR(st_mode))
		mode = 'c';
	if (S_ISBLK(st_mode))
		mode = 'b';
	return mode;
}

static void safe_stat(const char *name)
{
	struct stat __stat, *stat = &__stat;
	int fd = open(name, O_RDONLY), ret = fstat(fd, stat);
	assert(fd >= 0 && ret == 0);
	printf("%c %3d   %4d %10d  ", getmode(stat->st_mode),
	       stat->st_nlinks, stat->st_blocks, stat->st_size);
}

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

static void safe_lsdir(int round)
{
	DIR *dirp = opendir(".");
	assert(dirp != NULL);
	struct dirent *direntp;
	while ((direntp = readdir(dirp)) != NULL) {
		printf("%d: ", round);
		safe_stat(direntp->name);
		printf("%s\n", direntp->name);
	}
	closedir(dirp);
}

void changedir(const char *path)
{
	static int round = 0;
	printf("------------------------\n");
	safe_chdir(path);
	safe_getcwd(round);
	safe_lsdir(round);
	round++;
}

static void safe_link(const char *from, const char *to)
{
	int ret = link(from, to);
	assert(ret == 0);
}

static void safe_unlink(const char *path)
{
	int ret = unlink(path);
	assert(ret == 0);
}

int main(void)
{
	int fd1, fd2;
	struct stat *stat;

	changedir("/testdir/test");
	fd1 = safe_open("testfile", O_RDWR | O_TRUNC);
	stat = safe_fstat(fd1);
	assert(stat->st_size == 0 && stat->st_nlinks == 1);

	safe_link("testfile", "orz");

	fd2 = safe_open("orz", O_RDONLY);
	stat = safe_fstat(fd1);
	assert(stat->st_size == 0 && stat->st_nlinks == 2);
	stat = safe_fstat(fd2);
	assert(stat->st_size == 0 && stat->st_nlinks == 2);

	const char *msg = "Hello world!!\n";
	static char buffer[1024];
	size_t len = strlen(msg);
	memcpy(buffer, msg, len);
	safe_write(fd1, buffer, len);

	changedir(".");
	memset(buffer, 0, sizeof(buffer));
	safe_read(fd2, buffer, len);
	assert(strncmp(buffer, msg, len) == 0);

	cprintf("link test ok.\n");

	safe_unlink("orz");
	stat = safe_fstat(fd2);
	assert(stat->st_size == len && stat->st_nlinks == 1);
	close(fd2);
	changedir(".");

	cprintf("unlink test ok.\n");

	fd2 = safe_open("testfile", O_RDWR | O_TRUNC);
	safe_write(fd1, buffer, len);

	stat = safe_fstat(fd2);
	assert(stat->st_size == len * 2 && stat->st_nlinks == 1);

	fd2 = safe_open("testfile", O_RDWR | O_TRUNC);

	assert(link("test", "..") != 0);
	assert(link("testfile", ".") != 0);
	assert(link("testfile", "./testfile") != 0);
	assert(unlink(".") != 0 && unlink("..") != 0);

	changedir(".");
	cprintf("sfs_filetest3 pass.\n");
	return 0;
}
