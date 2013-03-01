#include <nios2_timer.h>

volatile unsigned long ticks;

/* *
 * clock_init - initialize 8253 clock to interrupt 100 times per second,
 * and then enable IRQ_TIMER.
 * */
void clock_init(void)
{
	nios2_timer_init();
}

void usleep(int usec)
{
	nios2_timer_usleep(usec);
}
