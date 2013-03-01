/*
 * =====================================================================================
 *
 *       Filename:  fcntl_glue.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/31/2012 02:09:47 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <linux/fs.h>
#include <linux/file.h>

#define _TODO_() printk(KERN_ALERT "TODO %s\n", __func__)
//#define _TODO_()

void kill_fasync(struct fasync_struct **fp, int sig, int band)
{
	_TODO_();
}

/*
 * fasync_helper() is used by almost all character device drivers
 * to set up the fasync queue, and for regular files by the file
 * lease code. It returns negative on error, 0 if it did no changes
 * and positive if it added/deleted the entry.
 */
int fasync_helper(int fd, struct file *filp, int on,
		  struct fasync_struct **fapp)
{
	_TODO_();
	return -EINVAL;
}
