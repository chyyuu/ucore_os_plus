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
//void lapic_ap_start(int apicid, uint32_t addr);
//int lapic_ipi_issue(int lapic_id);
//int lapic_ipi_issue_spec(int lapic_id, uint8_t vector);

struct lapic_chip{
	void (*cpu_init)(struct lapic_chip*);
	uint32_t (*id)(struct lapic_chip*);
	void (*eoi)(struct lapic_chip*);
	void (*start_ap)(struct lapic_chip*, struct cpu*, uint32_t addr);
	void *private_data;
};

struct lapic_chip* x2apic_lapic_init(void);
struct lapic_chip* xapic_lapic_init(void);

struct lapic_chip *lapic_get_chip();

/* helper macros */
#define lapic_eoi() do{struct lapic_chip* __c = lapic_get_chip(); \
	__c->eoi(__c);}while(0)

#define lapic_start_ap(cpuid, addr) do{struct lapic_chip* __c = lapic_get_chip(); \
	assert(__c->start_ap != NULL); \
	__c->start_ap(__c, cpuid, addr);}while(0)

#endif
