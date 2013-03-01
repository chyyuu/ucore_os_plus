#include <pmm.h>
#include <arch.h>

// invalidate a TLB entry, but only if the page tables being
// edited are the ones currently in use by the processor.
void tlb_invalidate(pgd_t * pgdir, uintptr_t la)
{
	if (rcr3() == PADDR(pgdir)) {
		invlpg((void *)la);
	}
}

/**
 * set_pgdir - save the physical address of the current pgdir
 */
void set_pgdir(struct proc_struct *proc, pgd_t * pgdir)
{
	assert(proc != NULL);
	proc->cr3 = PADDR(pgdir);
}

/**
 * load_pgdir - use the page table specified in @proc by @cr3
 */
void load_pgdir(struct proc_struct *proc)
{
	if (proc != NULL)
		lcr3(proc->cr3);
	else
		lcr3(PADDR(init_pgdir_get()));
}

/**
 * map_pgdir - map the current pgdir @pgdir to its own address space
 */
void map_pgdir(pgd_t * pgdir)
{
	ptep_map(&(pgdir[PGX(VPT)]), PADDR(pgdir));
	ptep_set_s_write(&(pgdir[PGX(VPT)]));
}
