#ifndef __KERN_FS_SYSFILE_H__
#define __KERN_FS_SYSFILE_H__

#include <types.h>

struct stat;
struct dirent;

int sysfile_open(const char *path, uint32_t open_flags);
int sysfile_close(int fd);
int sysfile_read(int fd, void *base, size_t len);
int sysfile_write(int fd, void *base, size_t len);
int sysfile_seek(int fd, off_t pos, int whence);
int sysfile_fstat(int fd, struct stat *stat);
int sysfile_stat(const char *fn, struct stat *stat);
int sysfile_fsync(int fd);
int sysfile_chdir(const char *path);
int sysfile_mkdir(const char *path);
int sysfile_link(const char *path1, const char *path2);
int sysfile_rename(const char *path1, const char *path2);
int sysfile_unlink(const char *path);
int sysfile_getcwd(char *buf, size_t len);
int sysfile_getdirentry(int fd, struct dirent *direntp, uint32_t * len);
int sysfile_dup(int fd1, int fd2);
int sysfile_pipe(int *fd_store);
int sysfile_mkfifo(const char *name, uint32_t open_flags);

int sysfile_ioctl(int fd, unsigned int cmd, unsigned long arg);
void *sysfile_linux_mmap2(void *addr, size_t len, int prot, int flags, int fd,
			  size_t pgoff);

#endif /* !__KERN_FS_SYSFILE_H__ */
