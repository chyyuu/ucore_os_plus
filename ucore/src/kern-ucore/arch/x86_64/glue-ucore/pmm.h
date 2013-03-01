#ifndef __GLUE_UCORE_PMM_H__
#define __GLUE_UCORE_PMM_H__

#include <glue_pmm.h>
#include <glue_mmu.h>
#include <glue_pgmap.h>
#include <memlayout.h>
#include <types.h>
#include <assert.h>
#include <atomic.h>
#include <proc.h>

extern struct segdesc pls_gdt[];

/* Simply translate between VA and PA without checking */
#define KADDR(addr) ((void*)((uintptr_t)(addr) + PBASE))
#define PADDR(addr) ((uintptr_t)(addr) - PBASE)

#define NEXT_PAGE(pg) (pa2page(page2pa(pg) + PGSIZE))

static inline ppn_t page2ppn(struct Page *page)
{
	return PPN(page->pa);
}

static inline uintptr_t page2pa(struct Page *page)
{
	return page->pa;
}

static inline struct Page *pa2page(uintptr_t pa)
{
	return (struct Page *)(kpage_private_get(pa));
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
	if (!ptep_present(&pte)) {
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

pgd_t *get_pgd(pgd_t * pgdir, uintptr_t la, bool create);
pud_t *get_pud(pgd_t * pgdir, uintptr_t la, bool create);
pmd_t *get_pmd(pgd_t * pgdir, uintptr_t la, bool create);
pte_t *get_pte(pgd_t * pgdir, uintptr_t la, bool create);

#define alloc_page() alloc_pages(1)
#define free_page(page) free_pages(page, 1)

struct Page *alloc_pages(size_t npages);
void free_pages(struct Page *base, size_t npages);
size_t nr_used_pages(void);

struct Page *get_page(pgd_t * pgdir, uintptr_t la, pte_t ** ptep_store);
void page_remove(pgd_t * pgdir, uintptr_t la);
int page_insert(pgd_t * pgdir, struct Page *page, uintptr_t la,
		pte_perm_t perm);

void tlb_invalidate(pgd_t * pgdir, uintptr_t la);
struct Page *pgdir_alloc_page(pgd_t * pgdir, uintptr_t la, uint32_t perm);
void unmap_range(pgd_t * pgdir, uintptr_t start, uintptr_t end);
void exit_range(pgd_t * pgdir, uintptr_t start, uintptr_t end);
int copy_range(pgd_t * to, pgd_t * from, uintptr_t start, uintptr_t end,
	       bool share);

void set_pgdir(struct proc_struct *proc, pgd_t * pgdir);
void load_pgdir(struct proc_struct *proc);
void map_pgdir(pgd_t * pgdir);

void pmm_init(void);
void pmm_init_ap(void);

#endif
