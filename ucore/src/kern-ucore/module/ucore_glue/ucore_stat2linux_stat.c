/*
 * =====================================================================================
 *
 *       Filename:  ucore_stat2linux_stat.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/24/2012 05:05:28 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#include <linux/stat.h>
#include <linux/string.h>

#define __NO_UCORE_TYPE__
#define __NO_UCORE_STAT__
#include <stat.h>

int __ucore_linux_stat_getsize()
{
	return sizeof(struct stat);
}

int __ucore_stat2linux_stat(const struct ucore_stat *us, const struct stat *ls)
{
	memset(ls, 0, sizeof(struct stat));
	return sizeof(struct stat);
}
