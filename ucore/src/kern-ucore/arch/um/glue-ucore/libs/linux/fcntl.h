#ifndef __ARCH_UM_INCLUDE_LINUX_FCNTL_H__
#define __ARCH_UM_INCLUDE_LINUX_FCNTL_H__

#define HOST_O_ACCMODE	00000003
#define HOST_O_RDONLY	00000000
#define HOST_O_WRONLY	00000001
#define HOST_O_RDWR		00000002

/* for syscall 'lseek' */
#define SEEK_SET	0	/* Seek from beginning of file.  */
#define SEEK_CUR	1	/* Seek from current position.  */
#define SEEK_END	2	/* Seek from end of file.  */

#endif /* !__ARCH_UM_INCLUDE_LINUX_FCNTL_H__ */
