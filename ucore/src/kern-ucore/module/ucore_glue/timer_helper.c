/*
 * =====================================================================================
 *
 *       Filename:  timer_helper.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/15/2012 07:33:08 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <proc.h>
#include <sched.h>

void __ucore_add_timer(void *linux_timer, int expires, unsigned long data,
		       void (*function) (unsigned long))
{
	timer_t *t = (timer_t *) kmalloc(sizeof(timer_t));
	if (!t) {
		panic("failed to add linux timer");
		return;
	}
	timer_init(t, NULL, expires);
	t->linux_timer.linux_timer = linux_timer;
	t->linux_timer.data = data;
	t->linux_timer.function = function;
	add_timer(t);
	kprintf("__ucore_add_timer: %d\n", expires);
}
