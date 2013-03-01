/*
 * =====================================================================================
 *
 *       Filename:  ucore_helper.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/20/2012 08:33:46 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef __MODULE_UCORE_HELPER_H
#define __MODULE_UCORE_HELPER_H

/* timer_helper.c */
void __ucore_add_timer(void *linux_timer, int expires, unsigned long data,
		       void (*function) (unsigned long));

/* kthread_helper.c */
int __ucore_kernel_thread(int (*fn) (void *), const char *name, void *arg,
			  unsigned int clone_flags);

/* wait_helper.c */
void __ucore_wait_self();
int __ucore_wakeup_by_pid(int pid);

#endif
