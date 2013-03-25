#ifndef __LIBS_STAT_H__
#define __LIBS_STAT_H__

#include <types.h>

#ifndef __NO_UCORE_STAT__
struct stat {
#else
struct ucore_stat {
#endif
	uint32_t st_mode;	// protection mode and file type
	size_t st_nlinks;	// number of hard links
	size_t st_blocks;	// number of blocks file is using
	size_t st_size;		// file size (bytes)
};

struct linux_stat {
#if defined(__ARMEB__)
	unsigned short st_dev;
	unsigned short __pad1;
#else
	unsigned long st_dev;
#endif
	unsigned long st_ino;
	unsigned short st_mode;
	unsigned short st_nlink;
	unsigned short st_uid;
	unsigned short st_gid;
#if defined(__ARMEB__)
	unsigned short st_rdev;
	unsigned short __pad2;
#else
	unsigned long st_rdev;
#endif
	unsigned long st_size;
	unsigned long st_blksize;
	unsigned long st_blocks;
	unsigned long st_atime;
	unsigned long st_atime_nsec;
	unsigned long st_mtime;
	unsigned long st_mtime_nsec;
	unsigned long st_ctime;
	unsigned long st_ctime_nsec;
	unsigned long __unused4;
	unsigned long __unused5;
};

struct linux_stat64 {
	unsigned long long st_dev;
	unsigned char __pad0[4];
	unsigned long __st_ino;
	unsigned int st_mode;
	unsigned int st_nlink;
	unsigned int st_uid;
	unsigned int st_gid;
	unsigned long long st_rdev;
	unsigned char __pad3[4];
	long long st_size;
	unsigned long st_blksize;
	unsigned long long st_blocks;
	unsigned long st_atime;
	unsigned long st_atime_nsec;
	unsigned long st_mtime;
	unsigned long st_mtime_nsec;
	unsigned long st_ctime;
	unsigned long st_ctime_nsec;
	unsigned long long st_ino;
};

#if 0
#define S_IFMT          070000	// mask for type of file
#define S_IFREG         010000	// ordinary regular file
#define S_IFDIR         020000	// directory
#define S_IFLNK         030000	// symbolic link
#define S_IFCHR         040000	// character device
#define S_IFBLK         050000	// block device

#define S_ISREG(mode)                   (((mode) & S_IFMT) == S_IFREG)	// regular file
#define S_ISDIR(mode)                   (((mode) & S_IFMT) == S_IFDIR)	// directory
#define S_ISLNK(mode)                   (((mode) & S_IFMT) == S_IFLNK)	// symlink
#define S_ISCHR(mode)                   (((mode) & S_IFMT) == S_IFCHR)	// char device
#define S_ISBLK(mode)                   (((mode) & S_IFMT) == S_IFBLK)	// block device
#endif

#if defined(__KERNEL__) || !defined(__GLIBC__) || (__GLIBC__ < 2)

#ifndef __NO_UCORE_STAT__
#define S_IFMT  00170000
#define S_IFSOCK 0140000
#define S_IFLNK	 0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000

#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#endif
#endif

#endif /* !__LIBS_STAT_H__ */
