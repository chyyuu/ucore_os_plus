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

#include <board.h>
#include <picirq.h>
#include <serial.h>
#include <pmm.h>
#include <clock.h>

static const char *message = "Initializing Pandaboard Board...\n";

static void put_string(const char *str)
{
	while (*str)
		serial_putc(*str++);
}

#define EXT_CLK 38400000

void board_init_early()
{
	put_string(message);

	pic_init_early();	// init interrupt controller
	//get clock
	uint32_t clock_idx = inw(0x4a306110);
	uint32_t clock_dpll_per = inw(0x4a00814c);
	kprintf("clock_idx: %08x, dpll_per: %08x\n", clock_idx, clock_dpll_per);
	uint32_t mul = (clock_dpll_per >> 8) & 0x7FF;
	uint32_t div = (clock_dpll_per & 0x3F) + 1;
	kprintf("PER_CLK = %dMhz\n", EXT_CLK / 1000 * mul / div / 1000);
}

void board_init()
{
	uint32_t mpu_base =
	    (uint32_t) __ucore_ioremap(CORTEX_A9_MPU_INSTANCE_BASE, 2 * PGSIZE,
				       0);
	pic_init2(mpu_base + 0x100, mpu_base + 0x1000);
	serial_init(PANDABOARD_UART0, PER_IRQ_BASE + PANDABOARD_UART3_IRQ);
	clock_init_arm(mpu_base + 0x600, PRIVATE_TIMER0_IRQ);
}

/* no nand */
int check_nandflash()
{
	return 0;
}

struct nand_chip *get_nand_chip()
{
	return NULL;
}
