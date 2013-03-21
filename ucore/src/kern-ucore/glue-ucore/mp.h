#ifndef __GLUE_UCORE_MP_H__
#define __GLUE_UCORE_MP_H__

#include <glue_mp.h>
#include <glue_memlayout.h>
#include <types.h>

/* maxinum cpu number */
#define NCPU 128
#define MAX_NUMA_NODES 16
#define MAX_NUMA_MEMS  16

struct numa_node{
	uint32_t hwid;
	int nr_cpus;
	int nr_mems;
	struct{
		uintptr_t base;
		uint64_t  length;
	}mems[MAX_NUMA_MEMS];
	int cpu_ids[NCPU];
};

extern int pls_lapic_id;
extern int pls_lcpu_idx;
extern int pls_lcpu_count;

extern volatile int ipi_raise[LAPIC_COUNT];

extern pgd_t *mpti_pgdir;
extern uintptr_t mpti_la;
extern volatile int mpti_end;

int mp_init(void);

struct mm_struct;

void kern_enter(int source);
void kern_leave(void);
void mp_set_mm_pagetable(struct mm_struct *mm);
void __mp_tlb_invalidate(pgd_t * pgdir, uintptr_t la);
void mp_tlb_invalidate(pgd_t * pgdir, uintptr_t la);
void mp_tlb_update(pgd_t * pgdir, uintptr_t la);

#endif
