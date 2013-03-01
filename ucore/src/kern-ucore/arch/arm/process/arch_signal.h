/*
 * =====================================================================================
 *
 *       Filename:  arch_signal.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/27/2012 06:49:50 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef __ARCH_ARM_ARCH_SIGNAL_H
#define __ARCH_ARM_ARCH_SIGNAL_H
#include <signal.h>

struct sigframe {
	uintptr_t pretcode;
	int sign;
	struct trapframe tf;
	sigset_t old_blocked;
	//there's fpstate in linux, but nothing here
	unsigned int retcode[2];
};

#endif
