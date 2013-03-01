/*
 * =====================================================================================
 *
 *       Filename:  resource.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/25/2012 12:01:18 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#ifndef _UCORE_PROC_RESOURCE_H
#define _UCORE_PROC_RESOURCE_H
typedef uint32_t linux_rlim_t;
struct linux_rlimit {
	linux_rlim_t rlim_cur;	/* Soft limit */
	linux_rlim_t rlim_max;	/* Hard limit (ceiling for rlim_cur) */
};

/* Transmute defines to enumerations.  The macro re-definitions are
   necessary because some programs want to test for operating system
   features with #ifdef RUSAGE_SELF.  In ISO C the reflexive
   definition is a no-op.  */

/* Kinds of resource limit.  */
enum __rlimit_resource {
	/* Per-process CPU limit, in seconds.  */
	RLIMIT_CPU = 0,
#define RLIMIT_CPU RLIMIT_CPU

	/* Largest file that can be created, in bytes.  */
	RLIMIT_FSIZE = 1,
#define	RLIMIT_FSIZE RLIMIT_FSIZE

	/* Maximum size of data segment, in bytes.  */
	RLIMIT_DATA = 2,
#define	RLIMIT_DATA RLIMIT_DATA

	/* Maximum size of stack segment, in bytes.  */
	RLIMIT_STACK = 3,
#define	RLIMIT_STACK RLIMIT_STACK

	/* Largest core file that can be created, in bytes.  */
	RLIMIT_CORE = 4,
#define	RLIMIT_CORE RLIMIT_CORE

	/* Largest resident set size, in bytes.
	   This affects swapping; processes that are exceeding their
	   resident set size will be more likely to have physical memory
	   taken from them.  */
	__RLIMIT_RSS = 5,
#define	RLIMIT_RSS __RLIMIT_RSS

	/* Number of open files.  */
	RLIMIT_NOFILE = 7,
	__RLIMIT_OFILE = RLIMIT_NOFILE,	/* BSD name for same.  */
#define RLIMIT_NOFILE RLIMIT_NOFILE
#define RLIMIT_OFILE __RLIMIT_OFILE

	/* Address space limit.  */
	RLIMIT_AS = 9,
#define RLIMIT_AS RLIMIT_AS

	/* Number of processes.  */
	__RLIMIT_NPROC = 6,
#define RLIMIT_NPROC __RLIMIT_NPROC

	/* Locked-in-memory address space.  */
	__RLIMIT_MEMLOCK = 8,
#define RLIMIT_MEMLOCK __RLIMIT_MEMLOCK

	/* Maximum number of file locks.  */
	__RLIMIT_LOCKS = 10,
#define RLIMIT_LOCKS __RLIMIT_LOCKS

	/* Maximum number of pending signals.  */
	__RLIMIT_SIGPENDING = 11,
#define RLIMIT_SIGPENDING __RLIMIT_SIGPENDING

	/* Maximum bytes in POSIX message queues.  */
	__RLIMIT_MSGQUEUE = 12,
#define RLIMIT_MSGQUEUE __RLIMIT_MSGQUEUE

	/* Maximum nice priority allowed to raise to.
	   Nice levels 19 .. -20 correspond to 0 .. 39
	   values of this resource limit.  */
	__RLIMIT_NICE = 13,
#define RLIMIT_NICE __RLIMIT_NICE

	/* Maximum realtime priority allowed for non-priviledged
	   processes.  */
	__RLIMIT_RTPRIO = 14,
#define RLIMIT_RTPRIO __RLIMIT_RTPRIO

	__RLIMIT_NLIMITS = 15,
	__RLIM_NLIMITS = __RLIMIT_NLIMITS
#define RLIMIT_NLIMITS __RLIMIT_NLIMITS
#define RLIM_NLIMITS __RLIM_NLIMITS
};

#endif
