#include <types.h>
#include <stdio.h>
#include <string.h>
#include <pmm.h>
#include <buddy_pmm.h>
#include <sync.h>
#include <error.h>
#include <spinlock.h>

// virtual address of physicall page array
struct Page *pages;
// amount of physical memory (in pages)
size_t npage = 0;

// virtual address of boot-time page directory
pgd_t *boot_pgdir = NULL;
// physical address of boot-time page directory
uintptr_t boot_cr3;

// physical memory management
const struct pmm_manager *pmm_manager;
spinlock_s pmm_lock;

//alloc_pages - call pmm->alloc_pages to allocate a continuous n*PAGESIZE memory 
struct Page *alloc_pages(size_t n)
{
	struct Page *page;
	bool intr_flag;
	local_intr_save(intr_flag);
	spinlock_acquire(&pmm_lock);
	{
		page = pmm_manager->alloc_pages(n);
	}
	spinlock_release(&pmm_lock);
	local_intr_restore(intr_flag);
	return page;
}

//free_pages - call pmm->free_pages to free a continuous n*PAGESIZE memory 
void free_pages(struct Page *base, size_t n)
{
	bool intr_flag;
	local_intr_save(intr_flag);
	spinlock_acquire(&pmm_lock);
	{
		pmm_manager->free_pages(base, n);
	}
	spinlock_release(&pmm_lock);
	local_intr_restore(intr_flag);
}

//nr_free_pages - call pmm->nr_free_pages to get the size (nr*PAGESIZE) 
//of current free memory
size_t nr_free_pages(void)
{
	size_t ret;
	bool intr_flag;
	local_intr_save(intr_flag);
	spinlock_acquire(&pmm_lock);
	{
		ret = pmm_manager->nr_free_pages();
	}
	spinlock_release(&pmm_lock);
	local_intr_restore(intr_flag);
	return ret;
}

struct Page *get_page(pgd_t * pgdir, uintptr_t la, pte_t ** ptep_store)
{
	pte_t *ptep = get_pte(pgdir, la, 0);
	if (ptep_store != NULL) {
		*ptep_store = ptep;
	}
	if (ptep != NULL && *ptep & PTE_P) {
		return pa2page(*ptep);
	}
	return NULL;
}

static inline void page_remove_pte(pgd_t * pgdir, uintptr_t la, pte_t * ptep)
{
	if (*ptep & PTE_P) {
		struct Page *page = pte2page(*ptep);
		if (page_ref_dec(page) == 0) {
			free_page(page);
		}
		*ptep = 0;
		tlb_invalidate(pgdir, la);
	}
}

void page_remove(pgd_t * pgdir, uintptr_t la)
{
	pte_t *ptep = get_pte(pgdir, la, 0);
	if (ptep != NULL) {
		page_remove_pte(pgdir, la, ptep);
	}
}

int page_insert(pgd_t * pgdir, struct Page *page, uintptr_t la, uint32_t perm)
{
	pte_t *ptep = get_pte(pgdir, la, 1);
	if (ptep == NULL) {
		return -E_NO_MEM;
	}
	page_ref_inc(page);
	if (*ptep & PTE_P) {
		struct Page *p = pte2page(*ptep);
		if (p == page) {
			page_ref_dec(page);
		} else {
			page_remove_pte(pgdir, la, ptep);
		}
	}
	*ptep = page2pa(page) | PTE_P | perm;
	tlb_invalidate(pgdir, la);
	return 0;
}

pgd_t *get_pgd(pgd_t * pgdir, uintptr_t la, bool create)
{
	return &pgdir[PGX(la)];
}

pud_t *get_pud(pgd_t * pgdir, uintptr_t la, bool create)
{
#if PUXSHIFT == PGXSHIFT
	return get_pgd(pgdir, la, create);
#else /* PUXSHIFT == PGXSHIFT */
	pgd_t *pgdp;
	if ((pgdp = get_pgd(pgdir, la, create)) == NULL) {
		return NULL;
	}
	if (!(*pgdp & PTE_P)) {
		struct Page *page;
		if (!create || (page = alloc_page()) == NULL) {
			return NULL;
		}
		set_page_ref(page, 1);
		uintptr_t pa = page2pa(page);
		memset(VADDR_DIRECT(pa), 0, PGSIZE);
		*pgdp = pa | PTE_U | PTE_W | PTE_P;
	}
	return &((pud_t *) VADDR_DIRECT(PGD_ADDR(*pgdp)))[PUX(la)];
#endif /* PUXSHIFT == PGXSHIFT */
}

pmd_t *get_pmd(pgd_t * pgdir, uintptr_t la, bool create)
{
#if PMXSHIFT == PUXSHIFT
	return get_pud(pgdir, la, create);
#else /* PMXSHIFT == PUXSHIFT */
	pud_t *pudp;
	if ((pudp = get_pud(pgdir, la, create)) == NULL) {
		return NULL;
	}
	if (!(*pudp & PTE_P)) {
		struct Page *page;
		if (!create || (page = alloc_page()) == NULL) {
			return NULL;
		}
		set_page_ref(page, 1);
		uintptr_t pa = page2pa(page);
		memset(VADDR_DIRECT(pa), 0, PGSIZE);
		*pudp = pa | PTE_U | PTE_W | PTE_P;
	}
	return &((pmd_t *) VADDR_DIRECT(PUD_ADDR(*pudp)))[PMX(la)];
#endif /* PMXSHIFT == PUXSHIFT */
}

pte_t *get_pte(pgd_t * pgdir, uintptr_t la, bool create)
{
#if PTXSHIFT == PMXSHIFT
	return get_pmd(pgdir, la, create);
#else /* PTXSHIFT == PMXSHIFT */
	pmd_t *pmdp;
	if ((pmdp = get_pmd(pgdir, la, create)) == NULL) {
		return NULL;
	}

	if (!(*pmdp & PTE_P)) {
		struct Page *page;
		if (!create || (page = alloc_page()) == NULL) {
			return NULL;
		}
		set_page_ref(page, 1);
		uintptr_t pa = page2pa(page);
		memset(VADDR_DIRECT(pa), 0, PGSIZE);
		*pmdp = pa | PTE_U | PTE_W | PTE_P;
	}

	return &((pte_t *) VADDR_DIRECT(PMD_ADDR(*pmdp)))[PTX(la)];
#endif /* PTXSHIFT == PMXSHIFT */
}
