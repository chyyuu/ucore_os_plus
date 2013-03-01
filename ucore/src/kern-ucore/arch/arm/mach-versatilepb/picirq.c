/*
 * =====================================================================================
 *
 *       Filename:  picirq.c
 *
 *    Description:  Vectored Interrupt Controller (VIC) PL190 support
 *
 *        Version:  1.0
 *        Created:  03/25/2012 08:40:17 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#include <stdlib.h>

#include <types.h>
#include <arm.h>
#include <picirq.h>
#include <clock.h>
#include <serial.h>
#include <intr.h>
#include <kio.h>
#include <memlayout.h>

#define VICINTENABLE (0x010)
#define VICINTENCLEAR (0x014)
#define  VICIRQSTATUS (0x000)

volatile size_t ticks = 0;

//TODO this code is mach-dependent

// Current IRQ mask
// static uint32_t irq_mask = 0xFFFFFFFF & ~(1 << INT_UART0);
static bool did_init = 0;
#define VIC_VBASE __io_address(VERSATILE_VIC_BASE)

void pic_disable(unsigned int irq)
{
	outw(VIC_VBASE + VICINTENCLEAR, (1 << irq));
}

void pic_enable(unsigned int irq)
{
	outw(VIC_VBASE + VICINTENABLE, (1 << irq));
}

/* pic_init
 * initialize the interrupt, but doesn't enable them */
void pic_init(void)
{
	if (did_init)
		return;

	did_init = 1;
	//disable all
	outw(VIC_VBASE + VICINTENCLEAR, ~0);
	kprintf("pic_init()\n");

}

void irq_handler()
{
	uint32_t status = inw(VIC_VBASE + VICIRQSTATUS);
//  kprintf(".. %08x\n", status);
	if (status & (1 << TIMER0_IRQ)) {
		//kprintf("@");
		ticks++;
		//assert(pls_read(current) != NULL);
		run_timer_list();
		clock_clear();
	}
	if (status & (1 << UART_IRQ)) {
		//if ((c = cons_getc()) == 13) {
		//  debug_monitor(tf);
		//}
		//else {
		extern void dev_stdin_write(char c);
		char c = cons_getc();
		dev_stdin_write(c);
		//}
		//kprintf("#");
		serial_clear();
	}

}

/* irq_clear
 * 	Clear a pending interrupt request
 *  necessary when handling an irq */
void irq_clear(unsigned int source)
{
}
