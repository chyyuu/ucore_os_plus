#ifndef __IOAPIC_H__
#define __IOAPIC_H__

#include <lapic.h>

typedef struct ioapic_s {
	int apic_id;
	uintptr_t phys;
	uint32_t intr_base;
} ioapic_s;

extern int ioapic_id_set[LAPIC_COUNT];
extern ioapic_s ioapic[LAPIC_COUNT];

int ioapic_init(void);
void ioapic_send_eoi(int apicid, int irq);
void ioapic_enable(int apicid, int irq, int cpunum);
void ioapic_disable(int apicid, int irq);

#endif
