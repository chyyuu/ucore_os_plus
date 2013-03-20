#ifndef __LAPIC_H__
#define __LAPIC_H__

#include <types.h>
#include <mmu.h>
#include <pmm.h>
#include <mp.h>

int lapic_init(void);
int lapic_init_ap(void);
//int lapic_id();
//void lapic_eoi_send(void);
void lapic_ap_start(int apicid, uint32_t addr);
//int lapic_ipi_issue(int lapic_id);
//int lapic_ipi_issue_spec(int lapic_id, uint8_t vector);

struct lapic_chip{
	void (*cpu_init)(struct lapic_chip*);
	uint32_t (*id)(struct lapic_chip*);
	void (*eoi)(struct lapic_chip*);
	void *private_data;
};

struct lapic_chip* x2apic_lapic_init(void);
struct lapic_chip* xapic_lapic_init(void);

struct lapic_chip *lapic_get_chip();
#endif
