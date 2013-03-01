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
#include <assert.h>
#include <memlayout.h>

enum {
	INTERRUPT_STATUS = 0x00,	// number of pending interrupts
	INTERRUPT_NUMBER = 0x04,
	INTERRUPT_DISABLE_ALL = 0x08,
	INTERRUPT_DISABLE = 0x0c,
	INTERRUPT_ENABLE = 0x10
};

//TODO this code is mach-dependent

// Current IRQ mask
// static uint32_t irq_mask = 0xFFFFFFFF & ~(1 << INT_UART0);
static bool did_init = 0;
#define VIC_VBASE __io_address(GOLDFISH_VIC_BASE)

void pic_disable(unsigned int irq)
{
	outw(VIC_VBASE + INTERRUPT_DISABLE, irq);
}

void pic_enable(unsigned int irq)
{
	outw(VIC_VBASE + INTERRUPT_ENABLE, irq);
}

struct irq_action {
	ucore_irq_handler_t handler;
	void *opaque;
};

struct irq_action actions[32];

void register_irq(int irq, ucore_irq_handler_t handler, void *opaque)
{
	if (irq > 31) {
		kprintf("WARNING: register_irq: irq>31\n");
		return;
	}
	actions[irq].handler = handler;
	actions[irq].opaque = opaque;
}

/* pic_init
 * initialize the interrupt, but doesn't enable them */
void pic_init(void)
{
	if (did_init)
		return;

	did_init = 1;
	//disable all
	outw(VIC_VBASE + INTERRUPT_DISABLE_ALL, ~0);
	kprintf("pic_init()\n");

}

void irq_handler()
{
	uint32_t pending = inw(VIC_VBASE + INTERRUPT_STATUS);
	uint32_t irq = 0;
	while (pending > 0) {
		irq = inw(VIC_VBASE + INTERRUPT_NUMBER);
		if (actions[irq].handler) {
			(*actions[irq].handler) (irq, actions[irq].opaque);
		} else {
			panic("unknown irq\n");
			pic_disable(irq);
		}
		pending = inw(VIC_VBASE + INTERRUPT_STATUS);
	}
#if 0
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
#endif
}

/* irq_clear
 * 	Clear a pending interrupt request
 *  necessary when handling an irq */
void irq_clear(unsigned int source)
{
}
