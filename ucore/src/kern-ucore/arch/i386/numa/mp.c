#include <mp.h>
#include <pmm.h>
#include <trap.h>
#include <spinlock.h>
#include <proc.h>
#include <kio.h>
#include <assert.h>
#include <arch.h>
#include <vmm.h>
#include <sysconf.h>

void *percpu_offsets[NCPU];
DEFINE_PERCPU_NOINIT(struct cpu, cpus);

#if 0
#define mp_debug(a ...) kprintf(a)
#else
#define mp_debug(a ...)
#endif

int mp_init(void)
{
	sysconf.lcpu_boot = 0;
	sysconf.lnuma_count = 0;
	sysconf.lcpu_count = 1;
	percpu_offsets[0] = __percpu_start;

	return 0;
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
