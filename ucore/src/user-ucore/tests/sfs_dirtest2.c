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

	changedir("/testdir/test"); {
		safe_mkdir("/testdir/test/dir0/");
		assert(mkdir("testfile") != 0 && mkdir("../test/dir0/") != 0);
		assert(mkdir("dir0/dir1/dir2") != 0);
		safe_mkdir("dir0/dir1/");

		fd1 = safe_open("file1", O_CREAT | O_RDWR | O_EXCL);
		fd2 = safe_open("file1", O_CREAT | O_RDWR);

		stat = safe_fstat(fd1);
		assert(stat->st_nlinks == 1);
		stat = safe_fstat(fd2);
		assert(stat->st_nlinks == 1);

		assert(open("file1", O_CREAT | O_EXCL) < 0);
		changedir(".");

		safe_link("file1", "dir0/dir1/file2");
		changedir(".");

		stat = safe_fstat(fd1);
		assert(stat->st_nlinks == 2);
	}

	changedir("dir0/dir1"); {
		assert(unlink("dir0/dir1") != 0);

		safe_unlink("/testdir/test/dir0/dir1/file2");
		safe_unlink("/testdir/test/dir0/dir1");

		stat = safe_fstat(fd1);
		assert(stat->st_nlinks == 1);
	}

	changedir("/testdir/test/dir0"); {
		safe_unlink("../file1");
		safe_unlink("../dir0");
	}

	changedir("/testdir/test");
	printf("sfs_dirtest2 pass.\n");
	return 0;
}
