#include <mp.h>
#include <trap.h>
#include <spinlock.h>
#include <proc.h>
#include <kio.h>
#include <assert.h>

PLS int pls_lapic_id;
PLS int pls_lcpu_idx;
PLS int pls_lcpu_count;

static spinlock_s kern_lock = { 0 };

PLS static int volatile pls_local_kern_locking;

volatile int ipi_raise[LAPIC_COUNT] = { 0 };

#if 0
#define mp_debug(a ...) kprintf(a)
#else
#define mp_debug(a ...)
#endif

int mp_init(void)
{
	pls_write(lapic_id, lapic_id_get());
	pls_write(lcpu_idx, lcpu_idx_get());
	pls_write(lcpu_count, lcpu_count_get());

	pls_write(local_kern_locking, 0);

	return 0;
}

void kern_enter(int source)
{
	int flags;
	int lapic_id = pls_read(lapic_id);

	local_intr_save_hw(flags);
	if (pls_read(local_kern_locking) != 0) {
		mp_debug("CPU %d ERROR %d %d\n", lapic_id, source,
			 (read_rflags() & FL_IF) != 0);
		assert(0);
	}

	pls_write(local_kern_locking, 1);
	mp_debug("CPU %d PID %d ENTERING KERN %d %d\n", lapic_id,
		 pls_read(current) == NULL ? -1 : pls_read(current)->pid,
		 source, (read_rflags() & FL_IF) != 0);
	while (1) {
		if (ipi_raise[lapic_id]) {
			asm volatile ("int %0"::"n" (T_IPI));
		}

		if (spinlock_acquire_try(&kern_lock))
			break;
	}
	mp_debug("CPU %d ENTERED KERN\n", lapic_id);
	pls_write(local_kern_locking, 0);
	local_intr_restore_hw(flags);
}

void kern_leave(void)
{
	int flags;
	local_intr_save_hw(flags);
	mp_debug("CPU %d LEAVING KERN\n", pls_read(lapic_id));
	spinlock_release(&kern_lock);
	local_intr_restore_hw(flags);
}

#include <glue_mp.h>
#include <glue_pmm.h>
#include <glue_memlayout.h>
#include <pmm.h>
#include <vmm.h>

void mp_set_mm_pagetable(struct mm_struct *mm)
{
	pgd_t *pgd;
	int flags;

	local_intr_save_hw(flags);

	/* XXX: kernel thread */
	if (mm == NULL) {
		pgd = init_pgdir_get();
	} else {
		pgd = mm->pgdir;
		pgd[NPGENTRY - 1] = init_pgdir_get()[NPGENTRY - 1];
	}

	lcr3(PADDR_DIRECT(pgd));

	assert(lapic_id_get() == pls_read(lapic_id));

	local_intr_restore_hw(flags);
}

pgd_t *mpti_pgdir;
uintptr_t mpti_la;
volatile int mpti_end;

#include <entry.h>

void mp_tlb_invalidate(pgd_t * pgdir, uintptr_t la)
{
	int lcpu_idx = pls_read(lcpu_idx);
	int lcpu_count = pls_read(lcpu_count);

	if (init_finished < lcpu_count) {
		tlb_invalidate(pgdir, la);
		return;
	}

	int flag;

	local_intr_save_hw(flag);
	// kprintf("LCPU %d ALL IPI START %p\n", lcpu_idx, ipi_raise);

	mpti_end = 0;

	mpti_pgdir = pgdir;
	mpti_la = la;

	int i;
	for (i = 0; i != lcpu_count; ++i)
		if (i != lcpu_idx) {
			/* XXX: idx=>lapic_id MAP!!!??? */
			int lapic_id = i;
			ipi_raise[lapic_id] = 1;
			lapic_ipi_issue(lapic_id);
		}
	// kprintf("LCPU %d ALL IPI ENDING\n", lcpu_idx);
	tlb_invalidate(pgdir, la);
	mpti_end = 1;

	for (i = 0; i != lcpu_count; ++i)
		if (i != lcpu_idx) {
			int lapic_id = i;
			while (ipi_raise[lapic_id]) ;
		}
	// kprintf("LCPU %d ALL IPI END\n", lcpu_idx);

	local_intr_restore_hw(flag);
}

void mp_tlb_update(pgd_t * pgdir, uintptr_t la)
{
	mp_tlb_invalidate(pgdir, la);
}
