#include <arm.h>
#include <clock.h>
#include <trap.h>
#include <stdio.h>
#include <kio.h>
#include <picirq.h>
#include <board.h>

volatile size_t ticks;

//MCK=133Mhz
#define PIV_1_MS              8313
#define PIV_10_MS              83130

/* 
 * Periodic Interval Timer initialization 
 */

void clock_init(void)
{
	outw(AT91C_BASE_PIT + PIT_MR, AT91C_SYSC_PITEN | AT91C_SYSC_PITIEN | PIV_10_MS);	//enable IRQ PIT
	pic_enable(AT91C_ID_SYS);
	kprintf("++ setup timer interrupts\n");
}

void clock_clear(void)
{
	inw(AT91C_BASE_PIT + PIT_PIVR);
}
