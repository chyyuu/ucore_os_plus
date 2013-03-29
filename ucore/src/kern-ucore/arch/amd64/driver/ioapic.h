#ifndef __IOAPIC_H__
#define __IOAPIC_H__

#include <arch.h>
#include <lapic.h>

typedef struct ioapic_s {
	uint32_t hwid;
	uintptr_t phys;
	int limit;
	uint32_t intr_base;
} ioapic_s;

extern ioapic_s ioapic[MAX_IOAPIC];
extern __ioapic_hwid_to_id[256];

int ioapic_init(void);
void ioapic_send_eoi(int id, int irq);
void ioapic_enable(int id, int irq, int cpunum);
void ioapic_disable(int id, int irq);
void ioapic_register_one(ioapic_s *);

#define ioapic_hwid_to_id(id) (__ioapic_hwid_to_id[id])

#endif
