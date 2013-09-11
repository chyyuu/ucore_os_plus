#ifndef __GLUE_UCORE_MP_H__
#define __GLUE_UCORE_MP_H__
#include "mplimits.h"
#include <memlayout.h>
#include <types.h>
#include <percpu.h>
#include <arch_cpu.h>
#include <list.h>
#include <spinlock.h>
#include <cpuset.h>
#define __padout__  \
  char __XCONCAT(__padout, __COUNTER__)[0] __attribute__((aligned(CACHELINE)))
#define __mpalign__ __attribute__((aligned(CACHELINE)))


struct cpu;
struct numa_node;


struct numa_node{
	uint32_t id;
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
	struct __arch_cpu  arch_data;

	__padout__;
	//percpu
	struct cpu *cpu;  //mysellf
	void *percpu_base;           // Per-CPU memory region base

	struct proc_struct *__current;
	struct proc_struct *idleproc;
} __mpalign__;

struct numa_mem_zone{
	uint32_t id;
	struct Page *page;
	size_t n;
	struct numa_node *node;
};

DECLARE_PERCPU(struct cpu, cpus);
extern struct numa_node numa_nodes[MAX_NUMA_NODES];
extern struct numa_mem_zone numa_mem_zones[MAX_NUMA_MEM_ZONES];

#include <arch_mp.h>

extern pgd_t *mpti_pgdir;
extern uintptr_t mpti_la;
extern volatile int mpti_end;

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

void cpu_up(int id);

#define myid() (mycpu()->id)

struct ipi_call{
	void *private_data;
	void (*callback)(struct ipi_call *data);
	atomic_t waiting;
	list_entry_t call_queue_link[NCPU]; /* percpu call queue link */
};

struct ipi_queue{
	spinlock_s lock;
	bool ipi_running;
	list_entry_t head;
};
DECLARE_PERCPU(struct ipi_queue, ipi_queues);

void ipi_init(void);
void do_ipicall(void);
void ipi_run_on_cpu(const cpuset_t *cs, void *data, void (*cb)(struct ipi_call*));
void fire_ipi_one(int cpuid);

#endif
