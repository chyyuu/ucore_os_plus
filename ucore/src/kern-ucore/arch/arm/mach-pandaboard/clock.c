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

#include <arm.h>
#include <board.h>
#include <clock.h>
#include <trap.h>
#include <stdio.h>
#include <kio.h>
#include <picirq.h>

#define PRESCALER_VAL 0
/* 10ms */
#define LOAD_VALUE    (7680000/2-1)

#define TIMER_LOAD    0x00
#define TIMER_CONTROL 0x08
#define TIMER_ISR     0x0C

#define TIMER_CONTROL_VAL ((PRESCALER_VAL << 8)|0x7)

static uint32_t timer_base = 0;

void clock_clear(void)
{
	outw(timer_base + TIMER_ISR, 0x01);
}

static int clock_int_handler(int irq, void *data)
{
	__common_timer_int_handler();
	clock_clear();
	return 0;
}

void clock_init_arm(uint32_t base, int irq)
{
	//TODO
	timer_base = base;
	outw(timer_base + TIMER_LOAD, LOAD_VALUE);
	outw(timer_base + TIMER_CONTROL, TIMER_CONTROL_VAL);
	register_irq(irq, clock_int_handler, 0);
	pic_enable(irq);
}
