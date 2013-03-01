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

#define TIMER0_VA_BASE       __io_address(GOLDFISH_TIMER0_1_BASE)
enum {
	TIMER_TIME_LOW = 0x00,	// get low bits of current time and update TIMER_TIME_HIGH
	TIMER_TIME_HIGH = 0x04,	// get high bits of time at last TIMER_TIME_LOW read
	TIMER_ALARM_LOW = 0x08,	// set low bits of alarm and activate it
	TIMER_ALARM_HIGH = 0x0c,	// set high bits of next alarm
	TIMER_CLEAR_INTERRUPT = 0x10,
	TIMER_CLEAR_ALARM = 0x14
};

#define TIMER0_INTERVAL  10000000	//10ms

static void reload_timer()
{
	unsigned long long t = 0;
	t = inw(TIMER0_VA_BASE + TIMER_TIME_LOW);
	t |= (long long)inw(TIMER0_VA_BASE + TIMER_TIME_HIGH) << 32;
	t += TIMER0_INTERVAL;
	outw(TIMER0_VA_BASE + TIMER_ALARM_HIGH, t >> 32);
	outw(TIMER0_VA_BASE + TIMER_ALARM_LOW, t);
}

void clock_clear(void)
{
	outw(TIMER0_VA_BASE + TIMER_CLEAR_INTERRUPT, 1);
}

static int clock_int_handler(int irq, void *data)
{
	__common_timer_int_handler();
	reload_timer();
	clock_clear();
	return 0;
}

void clock_init_arm(uint32_t base, int irq)
{
	clock_clear();
	reload_timer();
	register_irq(TIMER0_IRQ, clock_int_handler, 0);
	pic_enable(TIMER0_IRQ);
}
