#ifndef __ARCH_ARM_NUMA_ARCH_MP_H__
#define __ARCH_ARM_NUMA_ARCH_MP_H__

#include <types.h>
#include <percpu.h>

#define PERCPU_SECTION	__section__(".percpu")

static inline struct cpu* mycpu(void)
{
	return per_cpu_ptr(cpus, 0);
}

#endif /* __ARCH_ARM_NUMA_ARCH_MP_H__ */
