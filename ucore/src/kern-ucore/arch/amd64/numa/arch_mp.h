#ifndef __ARCH_AMD64_NUMA_ARCH_MP_H__
#define __ARCH_AMD64_NUMA_ARCH_MP_H__

#include <types.h>

#define LAPIC_COUNT 1
#define PERCPU_SECTION	__section__(".percpu,\"aw\",@nobits#")

static inline struct cpu* mycpu(void)
{
	uint64_t val;
	__asm volatile("movq %%gs:0, %0" : "=r" (val));
	return (struct cpu *)val;

}

static inline mp_lcr3(uintptr_t cr3)
{
	mycpu()->arch_data.tlb_cr3 = cr3;
	lcr3(cr3);
}

#endif /* __ARCH_AMD64_NUMA_ARCH_MP_H__ */
