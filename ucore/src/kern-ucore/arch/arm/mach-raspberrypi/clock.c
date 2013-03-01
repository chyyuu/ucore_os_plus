#include <arm.h>
#include <board.h>
#include <clock.h>
#include <trap.h>
#include <stdio.h>
#include <kio.h>
#include <picirq.h>

#define ARM_TIMER_LOD 0x2000B400
#define ARM_TIMER_CTL 0x2000B408
#define ARM_TIMER_CLI 0x2000B40C
#define ARM_TIMER_DIV 0x2000B41C

#define TIMER_INTERVAL  10000	//10ms

static void reload_timer()
{
	// ARM_TIMER_LOD: load: loaded into the time value register when written or counted to 0
	outw(ARM_TIMER_LOD, TIMER_INTERVAL - 1);
}

void clock_clear(void)
{
	// ARM_TIMER_CLI: clear: write anything to clear the pending interrupt
	outw(ARM_TIMER_CLI, 0);
}

static int clock_int_handler(int irq, void *data)
{
	__common_timer_int_handler();
	clock_clear();
	return 0;
}

void clock_init_arm(uint32_t base, int irq)
{
	// ARM_TIMER_DIV: pre-divider: clock = apb_clock / (x+1), 1MHz
	outw(ARM_TIMER_DIV, 0x000000F9);
	// ARM_TIMER_CTL: control:
	//     1: 23-bit counter, 5: interrupt on, 7: timer on
	//     3..2: prescale off, 23..16: prescale, freq = clock / (x+1)
	outw(ARM_TIMER_CTL, 0x003E00A2);

	clock_clear();
	reload_timer();
	register_irq(TIMER0_IRQ, clock_int_handler, 0);
	pic_enable(TIMER0_IRQ);
}
