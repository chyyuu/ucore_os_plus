/*
 * =====================================================================================
 *
 *       Filename:  board.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/24/2012 01:21:05 PM
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
#include <serial.h>
#include <picirq.h>
#include <at91-pmc.h>
#include <stdio.h>
#include <kio.h>

static const char *message = "Initializing AT91 Board...\n";

static void put_string(const char *str)
{
	while (*str)
		serial_putc(*str++);
}

static uint32_t at91_pll_rate(uint32_t freq, uint32_t reg)
{
	unsigned mul, div;

	div = reg & 0xff;
	mul = (reg >> 16) & 0x7ff;
	if (div && mul) {
		freq /= div;
		freq *= mul + 1;
	} else
		freq = 0;

	return freq;
}

static void print_clock_info(unsigned long main_clock)
{
	if (!main_clock) {
		kprintf("clock_info: failed to get clock info.\n");
		return;
	}
	unsigned long freq = at91_pll_rate(main_clock,
					   inw(AT91C_BASE_PMC + PMC_PLLAR));
	unsigned long mckr = inw(AT91C_BASE_PMC + PMC_MCKR);
	freq /= (1 << ((mckr & AT91_PMC_PLLADIV2) >> 12));	/* plla divisor by 2 */
	//mck <- plla
	unsigned long mck_rate_hz = (mckr & AT91_PMC_MDIV) == AT91SAM9_PMC_MDIV_3 ? freq / 3 : freq / (1 << ((mckr & AT91_PMC_MDIV) >> 8));	/* mdiv */

	kprintf("Clocks: CPU %u MHz, master %u MHz, main %u.%03u MHz\n",
		freq / 1000000, (unsigned)mck_rate_hz / 1000000,
		(unsigned)main_clock / 1000000,
		((unsigned)main_clock % 1000000) / 1000);
}

void board_init()
{
	put_string(message);
	pic_init();		// init interrupt controller
	extern void serial_init();
	serial_init();
	print_clock_info(12000000);	//12Mhz
#ifdef HAS_NANDFLASH
	extern int nandflash_hw_init(void);
	nandflash_hw_init();
#endif
#ifdef HAS_SDS
	extern int sds_hw_init(void);
	sds_hw_init();
#endif
}
