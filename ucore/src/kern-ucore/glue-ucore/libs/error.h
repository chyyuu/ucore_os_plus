#ifndef __LIBS_ERROR_H__
#define __LIBS_ERROR_H__

#if 0
/* kernel error codes -- keep in sync with list in lib/printfmt.c */
#define E_UNSPECIFIED       1	// Unspecified or unknown problem
#define E_BAD_PROC          2	// Process doesn't exist or otherwise
#define E_INVAL             3	// Invalid parameter
#define E_NO_MEM            4	// Request failed due to memory shortage
#define E_NO_FREE_PROC      5	// Attempt to create a new process beyond
#define E_FAULT             6	// Memory fault
#define E_SWAP_FAULT        7	// SWAP READ/WRITE fault
#define E_INVAL_ELF         8	// Invalid elf file
#define E_KILLED            9	// Process is killed
#define E_PANIC             10	// Panic Failure
#define E_TIMEOUT           11	// Timeout
#define E_TOO_BIG           12	// Argument is Too Big
#define E_NO_DEV            13	// No such Device
#define E_NA_DEV            14	// Device Not Available
#define E_BUSY              15	// Device/File is Busy
#define E_NOENT             16	// No Such File or Directory
#define E_ISDIR             17	// Is a Directory
#define E_NOTDIR            18	// Not a Directory
#define E_XDEV              19	// Cross Device-Link
#define E_UNIMP             20	// Unimplemented Feature
#define E_SEEK              21	// Illegal Seek
#define E_MAX_OPEN          22	// Too Many Files are Open
#define E_EXISTS            23	// File/Directory Already Exists
#define E_NOTEMPTY          24	// Directory is Not Empty
#endif

#define E_PERM		 1	/* Operation not permitted */
#define E_NOENT		 2	/* No such file or directory */
#define E_SRCH		 3	/* No such process */
#define E_INTR		 4	/* Interrupted system call */
#define E_IO		 5	/* I/O error */
#define E_NXIO		 6	/* No such device or address */
#define E_2BIG		 7	/* Argument list too long */
#define E_NOEXEC		 8	/* Exec format error */
#define E_BADF		 9	/* Bad file number */
#define E_CHILD		10	/* No child processes */
#define E_AGAIN		11	/* Try again */
#define E_NOMEM		12	/* Out of memory */
#define E_ACCES		13	/* Permission denied */
#define E_FAULT		14	/* Bad address */
#define E_NOTBLK		15	/* Block device required */
#define E_BUSY		16	/* Device or resource busy */
#define E_EXIST		17	/* File exists */
#define E_XDEV		18	/* Cross-device link */
#define E_NODEV		19	/* No such device */
#define E_NOTDIR		20	/* Not a directory */
#define E_ISDIR		21	/* Is a directory */
#define E_INVAL		22	/* Invalid argument */
#define E_NFILE		23	/* File table overflow */
#define E_MFILE		24	/* Too many open files */
#define E_NOTTY		25	/* Not a typewriter */
#define E_TXTBSY		26	/* Text file busy */
#define E_FBIG		27	/* File too large */
#define E_NOSPC		28	/* No space left on device */
#define E_SPIPE		29	/* Illegal seek */
#define E_ROFS		30	/* Read-only file system */
#define E_MLINK		31	/* Too many links */
#define E_PIPE		32	/* Broken pipe */
#define E_DOM		33	/* Math argument out of domain of func */
#define E_RANGE		34	/* Math result not representable */

//ucore
#define E_UNIMP   35
#define E_PANIC   36
#define E_KILLED  37
#define E_UNSPECIFIED  38
#define E_SWAP_FAULT   39

#define E_NO_MEM E_NOMEM
#define E_INVAL_ELF E_NOEXEC
#define E_NO_FREE_PROC E_BUSY
#define E_BAD_PROC E_SRCH
#define E_MAX_OPEN  E_MFILE
#define E_NA_DEV  E_BUSY
#define E_EXISTS  E_EXIST
#define E_TOO_BIG E_FBIG
#define E_NOTEMPTY  E_ISDIR
#define E_NO_DEV  E_NODEV
#define E_TIMEOUT  E_BUSY
#define E_SEEK    E_SPIPE

/* the maximum allowed */
#define MAXERROR            39

#endif /* !__LIBS_ERROR_H__ */
