/*
 * =====================================================================================
 *
 *       Filename:  linux_misc_struct.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/31/2012 08:29:21 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef LINUX_MISC_STRUCT_H
#define LINUX_MISC_STRUCT_H

#include <types.h>

struct linux_timeval {
	long tv_sec;		/* seconds */
	long tv_usec;		/* microseconds */
};

struct linux_timezone {
	int tz_minuteswest;	/* minutes west of Greenwich */
	int tz_dsttime;		/* type of DST correction */
};

int ucore_gettimeofday(struct linux_timeval *tv, struct linux_timezone *tz);

#endif
