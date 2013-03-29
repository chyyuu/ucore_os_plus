/*
 * =====================================================================================
 *
 *       Filename:  arch_signal.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/27/2012 06:51:15 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <arch.h>
#include <mp.h>
#include <vmm.h>
#include <proc.h>
#include <slab.h>
#include <error.h>
#include "arch_signal.h"

// set user stack for signal handler, also set eip to handler
int __sig_setup_frame(int sign, struct sigaction *act, sigset_t oldset,
		      struct trapframe *tf)
{
	struct mm_struct *mm = current->mm;
	uintptr_t stack = current->signal_info.sas_ss_sp;
	if (stack == 0) {
		stack = tf->tf_esp;
	}
	struct sigframe *kframe = kmalloc(sizeof(struct sigframe));
	if (!kframe)
		return -E_NO_MEM;
	memset(kframe, 0, sizeof(struct sigframe));

	kframe->sign = sign;
	kframe->tf = *tf;
	kframe->old_blocked = oldset;

	/* mov r7, #119 */
	kframe->retcode[0] = 0xe3a07077;
	/* swi #0 */
	kframe->retcode[1] = 0xef000000;

	/* 4byte align */
	struct sigframe *frame =
	    (struct sigframe *)((stack - sizeof(struct sigframe)) & 0xfffffff8);
	lock_mm(mm);
	{
		if (!copy_to_user(mm, frame, kframe, sizeof(struct sigframe))) {
			unlock_mm(mm);
			kfree(kframe);
			return -E_INVAL;
		}
	}
	unlock_mm(mm);
	kfree(kframe);
	frame->pretcode = (uintptr_t) frame->retcode;

	tf->tf_regs.reg_r[0] = sign;
	tf->tf_epc = (uintptr_t) act->sa_handler;
	tf->tf_esp = (uintptr_t) frame;
	tf->__tf_user_lr = (uint32_t) frame->pretcode;
	return 0;
}

// do syscall sigreturn, reset the user stack and eip
int do_sigreturn()
{
	struct mm_struct *mm = current->mm;
	if (!current)
		return -E_INVAL;
	struct sighand_struct *sighand = current->signal_info.sighand;
	if (!sighand)
		return -E_INVAL;

	struct sigframe *kframe = kmalloc(sizeof(struct sigframe));
	if (!kframe)
		return -E_NO_MEM;

	struct sigframe *frame =
	    (struct sigframe *)(current->tf->tf_esp);
	lock_mm(mm);
	{
		if (!copy_from_user
		    (mm, kframe, frame, sizeof(struct sigframe), 0)) {
			unlock_mm(mm);
			kfree(kframe);
			return -E_INVAL;
		}
	}
	unlock_mm(mm);
	/* check the trapframe */
	if (trap_in_kernel(&kframe->tf)) {
		do_exit(-E_KILLED);
		return -E_INVAL;
	}

	lock_sig(sighand);
	current->signal_info.blocked = kframe->old_blocked;
	sig_recalc_pending(current);
	unlock_sig(sighand);

	*(current->tf) = kframe->tf;
	kfree(kframe);

	return 0;
}
