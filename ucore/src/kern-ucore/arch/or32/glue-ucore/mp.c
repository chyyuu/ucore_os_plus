#include <mp.h>
#include <pmm.h>
#include <trap.h>
#include <spinlock.h>
#include <proc.h>
#include <kio.h>
#include <assert.h>
#include <vmm.h>

PLS int pls_lapic_id;
PLS int pls_lcpu_idx;
PLS int pls_lcpu_count;

PLS static int volatile pls_local_kern_locking;

volatile int ipi_raise[LAPIC_COUNT] = { 0 };

#if 0
#define mp_debug(a ...) kprintf(a)
#else
#define mp_debug(a ...)
#endif

int mp_init(void)
{
	pls_write(lapic_id, 0);
	pls_write(lcpu_idx, 0);
	pls_write(lcpu_count, 1);

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
	extern uintptr_t boot_pgdir_pa;
	extern uintptr_t current_pgdir_pa;
	if (mm != NULL)
		current_pgdir_pa = PADDR(mm->pgdir);
	else
		current_pgdir_pa = boot_pgdir_pa;
	tlb_invalidate_all();
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
