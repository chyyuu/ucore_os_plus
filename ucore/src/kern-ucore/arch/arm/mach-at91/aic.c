/*
 * =====================================================================================
 *
 *       Filename:  aic.c
 *
 *    Description:  Basic driver for AIC @ at91
 *
 *        Version:  1.0
 *        Created:  03/30/2012 07:42:33 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Yang Yang
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#include <stdlib.h>

#include <types.h>
#include <arm.h>
#include <picirq.h>
#include <intr.h>
#include <kio.h>
#include <board.h>

#define AT91C_AIC_IRQ_COUNT 32
#define RTTC_INTERRUPT_LEVEL 0

static bool did_init = 0;

void irq_configure(unsigned irq, unsigned int priority, unsigned int src_type);

void pic_enable(unsigned int irq)
{
	outw(AT91C_BASE_AIC + AIC_IECR, 0x1 << irq);
}

void pic_disable(unsigned int irq)
{
	outw(AT91C_BASE_AIC + AIC_IDCR, 0x1 << irq);
}

/* pic_init
 * initialize the interrupt, but doesn't enable them */
void pic_init(void)
{
	if (did_init)
		return;
	did_init = 1;
	unsigned int i;

	for (i = 0; i < AT91C_AIC_IRQ_COUNT; i++) {
		outw(AT91C_BASE_AIC + AIC_SVR + 4 * i, i);

		outw(AT91C_BASE_AIC + AIC_SMR + 4 * i, 0);

		if (i < 8)
			outw(AT91C_BASE_AIC + AIC_EOICR, 0);
	}

	outw(AT91C_BASE_AIC + AIC_SPU, AT91C_AIC_IRQ_COUNT);
	outw(AT91C_BASE_AIC + AIC_DCR, 0);

	outw(AT91C_BASE_AIC + AIC_IDCR, 0xFFFFFFFF);
	outw(AT91C_BASE_AIC + AIC_ICCR, 0xFFFFFFFF);

	irq_configure(AT91C_ID_SYS, RTTC_INTERRUPT_LEVEL,
		      AT91C_AIC_SRCTYPE_INT_LEVEL_SENSITIVE);

}

/* irq_clear
 * 	Clear a pending interrupt request
 *  necessary when handling an irq */
void irq_clear(unsigned int source)
{
	outw(AT91C_BASE_AIC + AIC_ICCR, 0x1 << source);
}

void irq_configure(unsigned irq, unsigned int priority, unsigned int src_type)
{

	unsigned int mask = 0x1 << irq;

	outw(AT91C_BASE_AIC + AIC_IDCR, mask);
	outw(AT91C_BASE_AIC + AIC_SMR + 4 * irq, src_type | priority);
	outw(AT91C_BASE_AIC + AIC_ICCR, mask);
}
