#ifndef __KERN_MM_PMM_H__
#define __KERN_MM_PMM_H__

#include <types.h>
#include <mmu.h>
#include <memlayout.h>
#include <atomic.h>
#include <spinlock.h>
#include <assert.h>

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
extern spinlock_s pmm_lock;
extern pgd_t *boot_pgdir;
extern uintptr_t boot_cr3;

void pmm_init(void);

struct Page *alloc_pages(size_t n);
void free_pages(struct Page *base, size_t n);
size_t nr_free_pages(void);

#define alloc_page() alloc_pages(1)
#define free_page(page) free_pages(page, 1)

pgd_t *get_pgd(pgd_t * pgdir, uintptr_t la, bool create);
pud_t *get_pud(pgd_t * pgdir, uintptr_t la, bool create);
pmd_t *get_pmd(pgd_t * pgdir, uintptr_t la, bool create);
pte_t *get_pte(pgd_t * pgdir, uintptr_t la, bool create);
struct Page *get_page(pgd_t * pgdir, uintptr_t la, pte_t ** ptep_store);
void page_remove(pgd_t * pgdir, uintptr_t la);
int page_insert(pgd_t * pgdir, struct Page *page, uintptr_t la, uint32_t perm);

void load_rsp0(uintptr_t rsp0);
void tlb_invalidate(pgd_t * pgdir, uintptr_t la);

void print_pgdir(int (*printf) (const char *fmt, ...));

/* Simply translate between VA and PA without checking */
#define VADDR_DIRECT(addr) ((void*)((uintptr_t)(addr) + PHYSBASE))
#define PADDR_DIRECT(addr) ((uintptr_t)(addr) - PHYSBASE)

extern struct Page *pages;
extern size_t npage;

static inline ppn_t page2gidx(struct Page *page)
{
	return page - pages;
}

static inline ppn_t page2ppn(struct Page *page)
{
	return page2gidx(page) + (RESERVED_DRIVER_OS_SIZE / PGSIZE);
}

static inline uintptr_t page2pa(struct Page *page)
{
	return page2ppn(page) << PGSHIFT;
}

static inline struct Page *pa2page(uintptr_t pa)
{
	if (PPN(pa - RESERVED_DRIVER_OS_SIZE) >= npage) {
		panic("pa2page called with invalid pa");
	}
	return &pages[PPN(pa - RESERVED_DRIVER_OS_SIZE)];
}

static inline void *page2va(struct Page *page)
{
	return VADDR_DIRECT(page2pa(page));
}

static inline struct Page *va2page(void *va)
{
	return pa2page(PADDR_DIRECT(va));
}

static inline struct Page *pte2page(pte_t pte)
{
	if (!(pte & PTE_P)) {
		panic("pte2page called with invalid pte");
	}
	return pa2page(PTE_ADDR(pte));
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

extern struct pseudodesc gdt_pd;
extern char bootstack[], bootstacktop[];

#endif /* !__KERN_MM_PMM_H__ */
