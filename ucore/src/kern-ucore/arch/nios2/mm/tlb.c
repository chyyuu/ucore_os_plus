#include <tlb.h>
#include <nios2.h>
#include <system.h>
#include <io.h>
#include <mmu.h>
#include <pmm.h>
#include <arch.h>
#include <proc.h>
#include <kio.h>

#define current (pls_read(current))

static int way = 0;

void local_flush_tlb_all(void)
{
	int i;
	unsigned long vaddr = IO_REGION_BASE;
	unsigned int way;

	/* Map each TLB entry to physcal address 0 
	 * with no-access and a bad ptbase
	 */
	for (way = 0; way < TLB_NUM_WAYS; way++) {
		for (i = 0; i < TLB_NUM_ENTRIES / TLB_NUM_WAYS; i++) {
			NIOS2_WRITE_PTEADDR(PPN(vaddr) << 2);
			NIOS2_WRITE_TLBMISC(TLBMISC_WE | (way << 20));
			NIOS2_WRITE_TLBACC(PPN(MAX_PHYS_ADDR));
			vaddr += 1UL << 12;
		}
	}
}

#define TLBMISC_D 1

static inline void print_pgfault(uintptr_t la)
{
	int tlbmisc, estatus;
	NIOS2_READ_TLBMISC(tlbmisc);
	NIOS2_READ_ESTATUS(estatus);
	kprintf("page fault at 0x%08x: %c/%c [%s].\n", la,
		(estatus & NIOS2_STATUS_U_MSK) ? 'U' : 'K',
		(tlbmisc & TLBMISC_D) ? 'D' : 'I',
		/*(tf->tf_err & 1) ? "protection fault" : */ "no page found");
}

//tlb_init - There are some invaid terms in tlb after booting.
//         - Clear them by writing some 
void tlb_init(void)
{
	local_flush_tlb_all();
	way = 0;
}

void tlb_setpid(int pid)
{
	NIOS2_WRITE_TLBMISC((pid << 4));
}

#define PID ((current != NULL) ? current->pid : 0)

void tlb_write(uintptr_t la, uintptr_t pa, bool write)
{
	NIOS2_WRITE_PTEADDR(PPN(la) << 2);
	NIOS2_WRITE_TLBMISC(TLBMISC_WE | (way << 20) | (PID << 4));
	NIOS2_WRITE_TLBACC(PPN(pa) | TLBACC_C | TLBACC_R | TLBACC_X |
			   (write ? TLBACC_W : 0));
	way = (way + 1) % TLB_NUM_WAYS;
}

int tlb_miss_handler(uintptr_t la, bool perm)
{
	pte_t *ptep = get_pte(NIOS2_PGDIR, la, 0);
	if (perm || ptep == NULL || !(*ptep & PTE_P)) {
		//do_pgfault
		extern struct mm_struct *check_mm_struct;
		//print_pgfault(la);
		struct mm_struct *mm;
		if (check_mm_struct != NULL) {
			assert(pls_read(current) == pls_read(idleproc));
			mm = check_mm_struct;
		} else {
			if (current == NULL) {
				panic("unhandled page fault. current==NULL\n");
			}
			mm = current->mm;
		}

		if (mm != NULL) {
			int ret = do_pgfault(mm, perm ? 2 : 2, la);
			ptep = get_pte(NIOS2_PGDIR, la, 0);
		}
		if (ptep == NULL) {
			int ea;
			NIOS2_READ_EA(ea);
			int badaddr;
			NIOS2_READ_BADADDR(badaddr);
			print_trapframe(current->tf);

			kprintf
			    ("unhandled page fault. la=0x%x ea=0x%x nr_free_pages=%d badaddr=0x%x pid=%d nr_free_pages()=%d\n",
			     la, ea, nr_free_pages(), badaddr,
			     current ? current->pid : 0, nr_free_pages());
			return -1;
		}
	}
	if (*ptep & PTE_P) {
		tlb_write(la, *ptep, (*ptep & PTE_W));
	} else {
		int badaddr;
		NIOS2_READ_BADADDR(badaddr);

		print_trapframe(current->tf);
		kprintf
		    ("tlb_miss_handler : ptep is not present! la=0x%x nr_free_pages=%d badaddr=0x%x, ptep=0x%x *ptep=0x%x\n",
		     la, nr_free_pages(), badaddr, ptep, *ptep);
		return -1;
	}
	return 0;
}
