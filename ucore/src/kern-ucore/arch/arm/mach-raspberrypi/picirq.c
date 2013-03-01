/*
 * See page 113 of BCM2835 ARM peripherals:
 * * valid GPU interrupts are 29, 43..57
 * * valid ARM interrupts are 0..7
 * In this code,
 * * cpu 0..7 are mapped to irq 0..7, gpu 29 is mapped irq 29, gpu 43..57 are ignored.
 * * IRQ 8..28 and 30..31 are deemed invalid
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

static bool did_init = 0;
#define IRQ_PENDING_BASIC 0x2000B200
#define IRQ_PENDING_1 0x2000B204
#define IRQ_ENABLE_BASIC 0x2000B218
#define IRQ_ENABLE_1 0x2000B210
#define IRQ_DISABLE_BASIC 0x2000B224
#define IRQ_DISABLE_1 0x2000B21C

void pic_disable(unsigned int irq)
{
	if (irq <= 7) {
		outw(IRQ_DISABLE_BASIC, 1 << irq);
	} else if (irq == 29) {
		outw(IRQ_DISABLE_1, 1 << irq);
	} else {
		//for rationale, read top of this file
		kprintf("WARNING: pic_disable: irq>7 && irq!=29\n");
	}
}

void pic_enable(unsigned int irq)
{
	if (irq <= 7) {
		outw(IRQ_ENABLE_BASIC, 1 << irq);
	} else if (irq == 29) {
		outw(IRQ_ENABLE_1, 1 << irq);
	} else {
		//for rationale, read top of this file
		kprintf("WARNING: pic_enable: irq>7 && irq!=29\n");
	}
}

struct irq_action {
	ucore_irq_handler_t handler;
	void *opaque;
};

struct irq_action actions[32];

void register_irq(int irq, ucore_irq_handler_t handler, void *opaque)
{
	if (irq <= 7 || irq == 29) {
		actions[irq].handler = handler;
		actions[irq].opaque = opaque;
	} else {
		//for rationale, read top of this file
		kprintf("WARNING: register_irq: irq>7 && irq!=29\n");
	}
}

void pic_init(void)
{
	if (did_init)
		return;

	did_init = 1;
	//disable all
	outw(IRQ_DISABLE_BASIC, ~0);
	outw(IRQ_DISABLE_1, ~0);
	//enable IRQ : done in entry.S
	kprintf("pic_init()\n");

}

void irq_handler()
{
	uint32_t pending0;
	while ((pending0 = inw(IRQ_PENDING_BASIC)) != 0) {
		uint32_t irq;
		//for rationale of 7 and 29, read top of this file
		uint32_t firstbit0 = __builtin_ctz(pending0);
		if (firstbit0 <= 7) {
			irq = firstbit0;
		} else if (firstbit0 == 8) {
			uint32_t pending1 = inw(IRQ_PENDING_1);
			uint32_t firstbit1 = __builtin_ctz(pending1);
			if (firstbit1 == 29)
				irq = 29;
			else
				panic("invalid irq: gpu 0x%02x(%d)\n",
				      firstbit1, firstbit1);
		} else {
			panic("invalid basic pending register: 0x%08x\n",
			      pending0);
		}
		if (actions[irq].handler) {
			(*actions[irq].handler) (irq, actions[irq].opaque);
		} else {
			panic("unknown irq 0x%02x(%d)\n", irq, irq);
			pic_disable(irq);
		}
	}
}

void irq_clear(unsigned int source)
{
}
