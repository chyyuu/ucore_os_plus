#include <ulib.h>
#include <stdio.h>
#include <string.h>
#include <stat.h>
#include <dirent.h>
#include <file.h>
#include <dir.h>
#include <unistd.h>

#define printf(...)                 fprintf(1, __VA_ARGS__)

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

int main(void)
{
	changedir("/testdir");
	changedir("/./../././../../././.././../.././../testdir/testman/..");
	changedir("home");
	changedir("../testman");
	changedir("coreutils");
	changedir("../././.././../../testdir/testman");
	changedir("/testdir");
	printf("sfs_dirtest1 pass.\n");
	return 0;
}
