/*
 * =====================================================================================
 *
 *       Filename:  kthread_helper.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/18/2012 11:18:51 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#include <pmm.h>
#include <proc.h>

int __ucore_kernel_thread(int (*fn) (void *), const char *name, void *arg,
			  unsigned int clone_flags)
{
	int pid = ucore_kernel_thread(fn, arg, clone_flags);
	if (pid <= 0)
		return pid;
	struct proc_struct *proc = find_proc(pid);
	if (!proc)
		return -1;
	set_proc_name(proc, name);
	return pid;
}
