#include <arch.h>
#include <trap.h>
#include <stdio.h>
#include <picirq.h>
#include <sched.h>
#include <asm/mipsregs.h>

volatile size_t ticks;

#define TIMER0_INTERVAL  1000000

static void reload_timer()
{
	uint32_t counter = read_c0_count();
	counter += TIMER0_INTERVAL;
	write_c0_compare(counter);
}

int clock_int_handler(void *data)
{
	ticks++;
//  if( (ticks & 0x1F) == 0)
//    cons_putc('A');
	run_timer_list();
	reload_timer();
	return 0;
}

void clock_init(void)
{
	reload_timer();
	pic_enable(TIMER0_IRQ);
	kprintf("++setup timer interrupts\n");
}
