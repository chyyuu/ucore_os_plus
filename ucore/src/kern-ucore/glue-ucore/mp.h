#ifndef __GLUE_UCORE_MP_H__
#define __GLUE_UCORE_MP_H__

#include <glue_memlayout.h>
#include <types.h>
#include <arch.h>
#include <percpu.h>

#define LAPIC_COUNT 1

#ifndef CACHELINE
#warning CACHELINE not defined
#define CACHELINE 64
#endif

#define __padout__  \
  char __XCONCAT(__padout, __COUNTER__)[0] __attribute__((aligned(CACHELINE)))
#define __mpalign__ __attribute__((aligned(CACHELINE)))


struct cpu;
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

struct proc_struct;

//TODO padding needed
struct cpu {
	uint32_t id;  //index of cpus[]
	uint32_t hwid; //apic id

	struct numa_node *node;

	__padout__;
	//percpu
	struct cpu *cpu;  //mysellf
	void *percpu_base;           // Per-CPU memory region base

	struct proc_struct *current;
	struct proc_struct *idleproc;
} __mpalign__;

DECLARE_PERCPU(struct cpu, cpus);

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

//we use gs to access percpu variable
//setup in tls_init
void tls_init(struct cpu* cpu);
/* alloc percpu memory */
void percpu_init(void);

static inline struct cpu* mycpu(void)
{
	uint64_t val;
	__asm volatile("movq %%gs:0, %0" : "=r" (val));
	return (struct cpu *)val;

}

#define myid() (mycpu()->id)

#endif
