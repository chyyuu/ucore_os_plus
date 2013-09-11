#ifndef __ARCH_AMD64_NUMA_ARCH_CPU_H
#define __ARCH_AMD64_NUMA_ARCH_CPU_H
#include <arch.h>

#define MAX_GDT_ITEMS SEG_COUNT
struct __arch_cpu{
	struct taskstate ts;
	struct segdesc gdt[MAX_GDT_ITEMS];
	uintptr_t tlb_cr3;
};


#endif
