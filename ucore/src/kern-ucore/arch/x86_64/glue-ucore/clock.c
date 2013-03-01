#include <clock.h>
#include <glue_tick.h>

volatile size_t ticks;

void clock_init(void)
{
	ticks = 0;
	tick_init(100);
}

void clock_init_ap(void)
{
	tick_init(100);
}
