/*
 * =====================================================================================
 *
 *       Filename:  kthread_glue.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/11/2012 02:38:17 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/wakelock.h>

#include "ucore_helper.h"

#define _TODO_() printk(KERN_ALERT "TODO %s\n", __func__)

extern int __ucore_kernel_thread(int (*fn) (void *), const char *name,
				 void *arg, u32 clone_flags);

struct task_struct *kthread_create(int (*threadfn) (void *data),
				   void *data, const char namefmt[], ...)
{
	struct task_struct *ts =
	    kzalloc(sizeof(struct task_struct), GFP_KERNEL);
	if (!ts)
		return ERR_PTR(-ENOMEM);
	va_list args;
	va_start(args, namefmt);
	vsnprintf(ts->comm, sizeof(ts->comm), namefmt, args);
	va_end(args);
	int pid = __ucore_kernel_thread(threadfn, ts->comm, data, 0);
	if (pid <= 0) {
		kfree(ts);
		return ERR_PTR(-EINVAL);
	}
	ts->pid = pid;

	pr_debug("kthread_create: %s, func = %p\n", ts->comm, (void *)threadfn);
	return ts;
}
