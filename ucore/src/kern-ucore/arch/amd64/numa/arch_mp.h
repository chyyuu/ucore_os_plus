#ifndef __ARCH_AMD64_NUMA_ARCH_MP_H__
#define __ARCH_AMD64_NUMA_ARCH_MP_H__

#include <types.h>
#include <arch.h>

#define LAPIC_COUNT 1
#define MAX_GDT_ITEMS SEG_COUNT
#define PERCPU_SECTION	__section__(".percpu,\"aw\",@nobits#")

static inline struct cpu* mycpu(void)
{
	uint64_t val;
	__asm volatile("movq %%gs:0, %0" : "=r" (val));
	return (struct cpu *)val;

}

struct __arch_cpu{
	struct taskstate ts;
	struct segdesc gdt[MAX_GDT_ITEMS];
};

#endif /* __ARCH_AMD64_NUMA_ARCH_MP_H__ */
