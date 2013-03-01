#ifndef __KERN_MM_PMM_H__
#define __KERN_MM_PMM_H__

//#include <defs.h>
#include <mmu.h>
#include <memlayout.h>
#include <atomic.h>
#include <assert.h>
#include <system.h>
#include <glue_pgmap.h>
#include <mmu.h>
#include <proc.h>
#include <kio.h>
#include <nios2.h>

#define TEST_PAGE 0x80000000

//#define NEXT_PAGE(pg) (pa2page(page2pa(pg) + PGSIZE))
inline struct Page *NEXT_PAGE(struct Page *pg);

// pmm_manager is a physical memory management class. A special pmm manager - XXX_pmm_manager
// only needs to implement the methods in pmm_manager class, then XXX_pmm_manager can be used
// by ucore to manage the total physical memory space.
struct pmm_manager {
	const char *name;	// XXX_pmm_manager's name
	void (*init) (void);	// initialize internal description&management data structure
	// (free block list, number of free block) of XXX_pmm_manager 
	void (*init_memmap) (struct Page * base, size_t n);	// setup description&management data structcure according to
	// the initial free physical memory space 
	struct Page *(*alloc_pages) (size_t n);	// allocate >=n pages, depend on the allocation algorithm 
	void (*free_pages) (struct Page * base, size_t n);	// free >=n pages with "base" addr of Page descriptor structures(memlayout.h)
	 size_t(*nr_free_pages) (void);	// return the number of free pages 
	void (*check) (void);	// check the correctness of XXX_pmm_manager 
};

extern const struct pmm_manager *pmm_manager;
extern pde_t *boot_pgdir;
extern uintptr_t boot_cr3;

void pmm_init(void);
void pmm_init_ap(void);

struct Page *alloc_pages(size_t n);
void free_pages(struct Page *base, size_t n);
size_t nr_free_pages(void);
size_t nr_used_pages(void);

#define alloc_page() alloc_pages(1)
#define free_page(page) free_pages(page, 1)

#define NPAGES_SRAM ((SRAM_END + 1 - SRAM_BASE) / PGSIZE)
#define NPAGES_SDRAM ((SDRAM_END + 1 - SDRAM_BASE) / PGSIZE)

pgd_t *get_pgd(pgd_t * pgdir, uintptr_t la, bool create);
pud_t *get_pud(pgd_t * pgdir, uintptr_t la, bool create);
pmd_t *get_pmd(pgd_t * pgdir, uintptr_t la, bool create);
pte_t *get_pte(pgd_t * pgdir, uintptr_t la, bool create);

struct Page *get_page(pde_t * pgdir, uintptr_t la, pte_t ** ptep_store);
void page_remove(pde_t * pgdir, uintptr_t la);
int page_insert(pde_t * pgdir, struct Page *page, uintptr_t la, uint32_t perm);

void load_rsp0(uintptr_t rsp0);
void set_pgdir(struct proc_struct *proc, pde_t * pgdir);
void load_pgdir(struct proc_struct *proc);
void map_pgdir(pde_t * pgdir);

void load_esp0(uintptr_t esp0);
void tlb_invalidate(pde_t * pgdir, uintptr_t la);
struct Page *pgdir_alloc_page(pde_t * pgdir, uintptr_t la, uint32_t perm);
void unmap_range(pde_t * pgdir, uintptr_t start, uintptr_t end);
void exit_range(pde_t * pgdir, uintptr_t start, uintptr_t end);
int copy_range(pde_t * to, pde_t * from, uintptr_t start, uintptr_t end,
	       bool share);

void print_pgdir(int (*printf) (const char *fmt, ...));

/* *
 * PADDR - takes a kernel virtual address (an address that points above KERNBASE),
 * where the machine's maximum 256MB of physical memory is mapped and returns the
 * corresponding physical address.  It panics if you pass it a non-kernel virtual address.
 * */
#define PADDR(kva) ({                                                   \
            uintptr_t __m_kva = (uintptr_t)(kva);                       \
            if (__m_kva < KERNBASE) {                                   \
                panic("PADDR called with invalid kva %08lx", __m_kva);  \
            }                                                           \
            __m_kva - KERNBASE;                                         \
        })

/* *
 * KADDR - takes a physical address and returns the corresponding kernel virtual
 * address. It panics if you pass an invalid physical address.
 * */
#define KADDR(pa) ({                                                    \
            uintptr_t __m_pa = (pa);                                    \
            (void *) (__m_pa + KERNBASE);                               \
        })

extern struct Page *pages;
extern size_t npage;

static inline ppn_t page2ppn(struct Page *page)
{
	int n = page - pages;
	if (n < NPAGES_SDRAM)
		return PPN(SDRAM_BASE) + n;
	else
		return PPN(SRAM_BASE) + (n - NPAGES_SDRAM);
}

static inline uintptr_t page2pa(struct Page *page)
{
	return page2ppn(page) << PGSHIFT;
}

static struct Page *pa2page(uintptr_t pa)
{
	if (pa >= SDRAM_BASE && pa <= SDRAM_END) {	//pa is in sram
		return &pages[PPN(pa - SDRAM_BASE)];
	} else if (pa >= SRAM_BASE && pa <= SRAM_END) {	//pa is in sdram
		return &pages[NPAGES_SDRAM + PPN(pa - SRAM_BASE)];
	} else {
		int ra;
		NIOS2_READ_RA(ra);
		kprintf("ra=0x%x\n", ra);

		if (pls_read(current))
			print_trapframe(pls_read(current)->tf);
		panic("pa2page called with invalid pa : pa=0x%x", pa);
		return NULL;
	}
}

static inline void *page2kva(struct Page *page)
{
	return KADDR(page2pa(page));
}

static inline struct Page *kva2page(void *kva)
{
	return pa2page(PADDR(kva));
}

static inline struct Page *pte2page(pte_t pte)
{
	if (!(pte & PTE_P)) {
		panic("pte2page called with invalid pte");
	}
	return pa2page(PTE_ADDR(pte));
}

static inline struct Page *pde2page(pde_t pde)
{
	return pa2page(PDE_ADDR(pde));
}

static inline struct Page *pmd2page(pmd_t pmd)
{
	return pa2page(PMD_ADDR(pmd));
}

static inline struct Page *pud2page(pud_t pud)
{
	return pa2page(PUD_ADDR(pud));
}

static inline struct Page *pgd2page(pgd_t pgd)
{
	return pa2page(PGD_ADDR(pgd));
}

static inline int page_ref(struct Page *page)
{
	return atomic_read(&(page->ref));
}

static inline void set_page_ref(struct Page *page, int val)
{
	atomic_set(&(page->ref), val);
}

static inline int page_ref_inc(struct Page *page)
{
	return atomic_add_return(&(page->ref), 1);
}

static inline int page_ref_dec(struct Page *page)
{
	return atomic_sub_return(&(page->ref), 1);
}

static inline pgd_t *init_pgdir_get(void)
{
	return boot_pgdir;
}

extern char bootstack[], bootstacktop[];

#endif /* !__KERN_MM_PMM_H__ */
