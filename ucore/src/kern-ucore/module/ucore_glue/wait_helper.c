/*
 * =====================================================================================
 *
 *       Filename:  wait_helper.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/20/2012 08:21:58 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <error.h>
#include <proc.h>
#include <sched.h>
#include <mp.h>

void __ucore_wait_self()
{
	//kprintf("ucore_wait_self self=%d\n", current->pid);
	bool intr_flag;
	local_intr_save(intr_flag);
	current->state = PROC_SLEEPING;
	current->wait_state = WT_KERNEL_SIGNAL;
	local_intr_restore(intr_flag);
	schedule();
}

int __ucore_wakeup_by_pid(int pid)
{
	//kprintf("ucore_wakeup_by_pid %d\n", pid);
	struct proc_struct *proc = find_proc(pid);
	if (!proc)
		return -E_INVAL;
	bool flag;
	local_intr_save(flag);
	if (proc->state == PROC_ZOMBIE) {
		local_intr_restore(flag);
		return -E_INVAL;
	}
	if (proc->state == PROC_RUNNABLE)
		wakeup_proc(proc);
	local_intr_restore(flag);
	return 0;
}
