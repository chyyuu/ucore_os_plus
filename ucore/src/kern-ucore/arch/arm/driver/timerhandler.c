/*
 * =====================================================================================
 *
 *       Filename:  timerhandler.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/10/2012 07:49:13 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <arm.h>
#include <board.h>
#include <clock.h>

volatile size_t ticks = 0;

#if (defined UCONFIG_HAVE_LINUX_DDE_BASE) \
 || (defined UCONFIG_HAVE_LINUX_DDE36_BASE)
extern volatile uint64_t jiffies_64;
extern unsigned long volatile jiffies;
#endif

static int sched_enabled = 0;

void enable_timer_list()
{
	sched_enabled = 1;
}

void __common_timer_int_handler()
{
	ticks++;
#if (defined UCONFIG_HAVE_LINUX_DDE_BASE) \
 || (defined UCONFIG_HAVE_LINUX_DDE36_BASE)
	jiffies++;
	jiffies_64++;
#endif
	//if(ticks % 100 == 0)
	//  serial_putc('A');
	extern void run_timer_list();
	if (sched_enabled)
		run_timer_list();
}
