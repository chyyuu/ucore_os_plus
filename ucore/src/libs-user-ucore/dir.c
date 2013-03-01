#include <types.h>
#include <string.h>
#include <syscall.h>
#include <malloc.h>
#include <stat.h>
#include <dirent.h>
#include <file.h>
#include <dir.h>
#include <error.h>
#include <unistd.h>

DIR *opendir(const char *path)
{
	DIR *dirp;
	if ((dirp = malloc(sizeof(DIR))) == NULL) {
		return NULL;
	}
	if ((dirp->fd = open(path, O_RDONLY)) < 0) {
		goto failed;
	}
	struct stat __stat, *stat = &__stat;
	if (fstat(dirp->fd, stat) != 0 || !S_ISDIR(stat->st_mode)) {
		goto failed;
	}
	dirp->dirent.offset = 0;
	return dirp;

failed:
	free(dirp);
	return NULL;
}

struct dirent *readdir(DIR * dirp)
{
	if (sys_getdirentry(dirp->fd, &(dirp->dirent)) == 0) {
		return &(dirp->dirent);
	}
	return NULL;
}

void closedir(DIR * dirp)
{
	close(dirp->fd);
	free(dirp);
}

int chdir(const char *path)
{
	return sys_chdir(path);
}

int getcwd(char *buffer, size_t len)
{
	return sys_getcwd(buffer, len);
}

int mkdir(const char *path)
{
	return sys_mkdir(path);
}

int link(const char *old_path, const char *new_path)
{
	return sys_link(old_path, new_path);
}

int rename(const char *old_path, const char *new_path)
{
	return sys_rename(old_path, new_path);
}

int unlink(const char *path)
{
	return sys_unlink(path);
}
