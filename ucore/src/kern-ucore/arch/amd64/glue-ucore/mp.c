#include <mp.h>
#include <pmm.h>
#include <trap.h>
#include <spinlock.h>
#include <proc.h>
#include <kio.h>
#include <assert.h>
#include <arch.h>
#include <vmm.h>
#include <percpu.h>
#include <sysconf.h>

void *percpu_offsets[NCPU];
DEFINE_PERCPU_NOINIT(struct cpu, cpus);

volatile int ipi_raise[LAPIC_COUNT] = { 0 };

#if 0
#define mp_debug(a ...) kprintf(a)
#else
#define mp_debug(a ...)
#endif

void
tls_init(struct cpu *c)
{
	// Initialize cpu-local storage.
	writegs(KERNEL_DS);
	/* gs base shadow reg in msr */
	writemsr(MSR_GS_BASE, (uint64_t)&c->cpu);
	writemsr(MSR_GS_KERNBASE, (uint64_t)&c->cpu);
	c->cpu = c;
}

/* TODO alloc percpu var in the corresponding    NUMA nodes */
void percpu_init(void)
{
	size_t percpu_size = ROUNDUP(__percpu_end - __percpu_start, CACHELINE);
	if(!percpu_size || sysconf.lcpu_count<=1)
		return;
	int i;
	size_t totalsize = percpu_size * (sysconf.lcpu_count - 1);
	unsigned int pages = ROUNDUP_DIV(totalsize, PGSIZE);
	kprintf("percpu_init: alloc %d pages for percpu variables\n", pages);
	struct Page *p = alloc_pages(pages);
	assert(p != NULL);
	void *kva = page2kva(p);
	/* zero it out */
	memset(kva, 0, pages * PGSIZE);
	for(i=1;i<sysconf.lcpu_count;i++, kva += percpu_size)
		percpu_offsets[i] = kva;
}


void kern_enter(int source)
{
}

void kern_leave(void)
{
}

void mp_set_mm_pagetable(struct mm_struct *mm)
{
	if (mm != NULL && mm->pgdir != NULL)
		lcr3(PADDR(mm->pgdir));
	else
		lcr3(boot_cr3);
}

pgd_t *mpti_pgdir;
uintptr_t mpti_la;
volatile int mpti_end;

void mp_tlb_invalidate(pgd_t * pgdir, uintptr_t la)
{
	tlb_invalidate(pgdir, la);
}

void mp_tlb_update(pgd_t * pgdir, uintptr_t la)
{
	tlb_update(pgdir, la);
}
