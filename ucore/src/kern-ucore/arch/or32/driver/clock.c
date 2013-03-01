#include <clock.h>
#include <board.h>
#include <stdio.h>
#include <kio.h>
#include <arch.h>
#include <or32/spr_defs.h>

volatile size_t ticks = 0;

void clock_init(void)
{
	kprintf("++ setup timer interrupts\n");

	mtspr(SPR_TTMR, TIMER_FREQ | SPR_TTMR_IE | SPR_TTMR_RT);
	mtspr(SPR_TTCR, 0);
}
