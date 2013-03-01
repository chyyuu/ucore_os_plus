#ifndef __LIBS_DIRENT_H__
#define __LIBS_DIRENT_H__

#include <types.h>
#include <unistd.h>

struct dirent {
	unsigned long d_ino;	/* Inode number */
	unsigned long offset;	/* Offset to next linux_dirent */
	unsigned short d_reclen;	/* Length of this linux_dirent */
	char name[FS_MAX_FNAME_LEN + 1];	/* Filename (null-terminated) */
	/* length is actually (d_reclen - 2 -
	   offsetof(struct linux_dirent, d_name) */
	/*
	   char           pad;       // Zero padding byte
	   char           d_type;    // File type (only since Linux 2.6.4;
	   // offset is (d_reclen - 1))
	 */

};

#endif /* !__LIBS_DIRENT_H__ */
