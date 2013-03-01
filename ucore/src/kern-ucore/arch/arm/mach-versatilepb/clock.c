/*
 * =====================================================================================
 *
 *       Filename:  clock.c
 *
 *    Description:  SP804 support, versatilepb
 *
 *        Version:  1.0
 *        Created:  03/25/2012 08:39:30 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#include <stdlib.h>

#include <arm.h>
#include <trap.h>
#include <stdio.h>
#include <kio.h>
#include <picirq.h>
#include <board.h>

volatile size_t ticks;
#define TIMER0_VA_BASE       __io_address(VERSATILE_TIMER0_1_BASE)
#define TIMER1_VA_BASE      (__io_address(VERSATILE_TIMER0_1_BASE) + 0x20)
#define TIMER2_VA_BASE       __io_address(VERSATILE_TIMER2_3_BASE)
#define TIMER3_VA_BASE      (__io_address(VERSATILE_TIMER2_3_BASE) + 0x20)

#define TimerXLoad (0)
#define TimerXValue (4)
#define TimerXControl (8)
#define TimerXIntClr (0x0C)

#define LOAD_VALUE 0xff

/* *
 * clock_init - initialize TIMER 0to interrupt 100 times per second,
 * and then enable INT_TIMER0 (IRQ).
 * PCLK = 50 Mhz, divider = 1/16, prescaler = 24
 * */
void clock_init(void)
{
	outw(TIMER0_VA_BASE + TimerXControl, 0);
	//clear int output
	outw(TIMER0_VA_BASE + TimerXIntClr, ~0);
	//load value
	outw(TIMER0_VA_BASE + TimerXLoad, LOAD_VALUE);

	//periodic, 32bit, int, en , 1/16
	outw(TIMER0_VA_BASE + TimerXControl, 0xe6);

	//vic enable
	pic_enable(TIMER0_IRQ);
	kprintf("++ setup timer interrupts\n");
}

void clock_clear(void)
{
	outw(TIMER0_VA_BASE + TimerXIntClr, ~0);
}
