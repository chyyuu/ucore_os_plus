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

static void safe_mkdir(const char *path)
{
	int ret = mkdir(path);
	assert(ret == 0);
}

static void safe_rename(const char *from, const char *to)
{
	int ret = rename(from, to);
	assert(ret == 0);
}

static void safe_unlink(const char *path)
{
	int ret = unlink(path);
	assert(ret == 0);
}

int main(void)
{
	int fd;
	struct stat *stat;

	changedir("/testdir/test");
	safe_mkdir("dir0");
	safe_mkdir("dir0/dir1");
	fd = safe_open("dir0/file1", O_CREAT | O_RDWR | O_EXCL);

	const char *msg = "Hello world!!\n";
	static char buffer[1024];
	size_t len = strlen(msg);
	memcpy(buffer, msg, len);
	safe_write(fd, buffer, len);

	stat = safe_fstat(fd);
	assert(stat->st_size == len && stat->st_nlinks == 1);

	changedir("dir0/dir1");
	safe_rename("../file1", "file2");
	safe_write(fd, buffer, len);
	close(fd);
	changedir(".");

	fd = safe_open("file2", O_RDONLY);
	stat = safe_fstat(fd);
	assert(stat->st_size == len * 2);

	safe_rename("../dir1", "../../dir2");
	changedir("/testdir/test");

	safe_unlink("dir2/file2");
	safe_unlink("dir0");
	safe_unlink("dir2");

	changedir(".");

	printf("sfs_dirtest3 pass.\n");
	return 0;
}
