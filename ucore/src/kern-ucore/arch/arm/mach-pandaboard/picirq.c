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
#include <assert.h>

#define ICCICR     0x00		// CPU Interface Control Register (RW)
#define ICCPMR   0x04		// CPU Interface  Priority Mask Register (RW)
#define ICCIAR   0x0C
#define ICCEOIR  0x10

#define ICDDCR   0x000		// Distributor Control Register (RW)
#define ICDISER0  0x100		// Distributor  Interrupt Set Enable Registers (RW)
#define ICDICER0  0x180		// Distributor  Interrupt Set Enable Registers (RW)
#define ICDISPR  0x200		// Distributor  Set Pending Register (RW)
#define ICDIPR	   0x400	//  Distributor Interrupt Priority Register (RW)
#define ICDSGIR  0xF00		//  Distributor Software Generated Interrupt Register (WO)
#define ICDICFR0 0xC00
#define ICDIPTR0 0x800

#define MAX_IRQS_NR  128

static uint32_t gic_base = 0;
static uint32_t dist_base = 0;

//TODO this code is mach-dependent

// Current IRQ mask
// static uint32_t irq_mask = 0xFFFFFFFF & ~(1 << INT_UART0);
static bool did_init = 0;

void pic_disable(unsigned int irq)
{
}

void pic_enable(unsigned int irq)
{
	if (irq >= MAX_IRQS_NR)
		return;
	int off = irq / 32;
	int j = irq % 32;

	outw(gic_base + ICCICR, 0);	// Disable CPU Interface (it is already disabled, but nevermind)
	outw(dist_base + ICDDCR, 0);	// Disable Distributor (it is already disabled, but nevermind)

	outw(dist_base + ICDISER0 + off * 4, 1 << j);
	outw(dist_base + ICDIPTR0 + (irq & ~0x3), 0x01010101);

	outw(gic_base + ICCICR, 3);	// Enable CPU Interface
	outw(dist_base + ICDDCR, 1);	// Enable Distributor
}

struct irq_action {
	ucore_irq_handler_t handler;
	void *opaque;
};

struct irq_action actions[MAX_IRQS_NR];

void register_irq(int irq, ucore_irq_handler_t handler, void *opaque)
{
	if (irq >= MAX_IRQS_NR) {
		kprintf("WARNING: register_irq: irq>=%d\n", MAX_IRQS_NR);
		return;
	}
	actions[irq].handler = handler;
	actions[irq].opaque = opaque;
}

void pic_init_early()
{
}

void pic_init()
{
}

/* pic_init
 * initialize the interrupt, but doesn't enable them */
void pic_init2(uint32_t g_base, uint32_t d_base)
{
	if (did_init)
		return;

	did_init = 1;
	//disable all
	gic_base = g_base;
	dist_base = d_base;
	outw(gic_base + ICCICR, 0);	// Disable CPU Interface (it is already disabled, but nevermind)
	outw(dist_base + ICDDCR, 0);	// Disable Distributor (it is already disabled, but nevermind)
	outw(gic_base + ICCPMR, 0xffff);	// All interrupts whose priority is > 0xff are unmasked

	int i;
	for (i = 0; i < 32; i++) {
		outw(dist_base + ICDICER0 + (i << 2), ~0);
	}

	outw(gic_base + ICCICR, 3);	// Enable CPU Interface
	outw(dist_base + ICDDCR, 1);	// Enable Distributor

}

void irq_handler()
{
	//TODO
	uint32_t intnr = inw(gic_base + ICCIAR) & 0x3FF;
	if (actions[intnr].handler) {
		(*actions[intnr].handler) (intnr, actions[intnr].opaque);
	} else {
		panic("Unhandled HW IRQ %d\n", intnr);
	}

	//EOI
	outw(gic_base + ICCEOIR, intnr & 0x3FF);
}

/* irq_clear
 * 	Clear a pending interrupt request
 *  necessary when handling an irq */
void irq_clear(unsigned int source)
{
}
