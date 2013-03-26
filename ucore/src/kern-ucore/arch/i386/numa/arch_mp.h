#ifndef __ARCH_I386_NUMA_ARCH_MP_H__
#define __ARCH_I386_NUMA_ARCH_MP_H__

#include <types.h>
#include <percpu.h>

static inline struct cpu* mycpu(void)
{
	return per_cpu_ptr(cpus, 0);
}

#endif /* __ARCH_I386_NUMA_ARCH_MP_H__ */
