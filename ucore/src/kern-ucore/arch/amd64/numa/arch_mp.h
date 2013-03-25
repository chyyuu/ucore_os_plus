#ifndef __ARCH_AMD64_NUMA_ARCH_MP_H__
#define __ARCH_AMD64_NUMA_ARCH_MP_H__

#include <types.h>

#define LAPIC_COUNT 1

static inline struct cpu* mycpu(void)
{
	uint64_t val;
	__asm volatile("movq %%gs:0, %0" : "=r" (val));
	return (struct cpu *)val;

}

#endif /* __ARCH_AMD64_NUMA_ARCH_MP_H__ */
