#include <pmm.h>
#include <stdio.h>
#include <mmu.h>
#include <memlayout.h>
#include <error.h>
#include <buddy_pmm.h>
#include <stdio.h>
#include <sync.h>
#include <types.h>
#include <slab.h>
#include <swap.h>
#include <string.h>
#include <kio.h>
#include <glue_mp.h>

PLS static size_t pls_used_pages;
PLS list_entry_t pls_page_struct_free_list;

// virtual address of physicall page array
struct Page *pages;
// amount of physical memory (in pages)
size_t npage = 0;

// virtual address of boot-time page directory
pde_t *boot_pgdir = NULL;

// physical memory management
const struct pmm_manager *pmm_manager;

/**
 * call pmm->init_memmap to build Page struct for free memory  
 */
void init_memmap(struct Page *base, size_t n)
{
	pmm_manager->init_memmap(base, n);
}

/**
 * boot_map_segment - setup&enable the paging mechanism
 * @param la    linear address of this memory need to map (after x86 segment map)
 * @param size  memory size
 * @param pa    physical address of this memory
 * @param perm  permission of this memory
 */
void
boot_map_segment(pde_t * pgdir, uintptr_t la, size_t size, uintptr_t pa,
		 uint32_t perm)
{
	assert(PGOFF(la) == PGOFF(pa));
	size_t n = ROUNDUP(size + PGOFF(la), PGSIZE) / PGSIZE;
	la = ROUNDDOWN(la, PGSIZE);
	pa = ROUNDDOWN(pa, PGSIZE);
	for (; n > 0; n--, la += PGSIZE, pa += PGSIZE) {
		pte_t *ptep = get_pte(pgdir, la, 1);
		assert(ptep != NULL);
		*ptep = pa | PTE_P | perm;
	}
}

size_t nr_used_pages(void)
{
	return pls_read(used_pages);
}

void pmm_init_ap(void)
{
	list_entry_t *page_struct_free_list =
	    pls_get_ptr(page_struct_free_list);
	list_init(page_struct_free_list);
	pls_write(used_pages, 0);
}

/**************************************************
 * Page allocations.
 **************************************************/

/**
 * call pmm->alloc_pages to allocate a continuing n*PAGESIZE memory
 * @param n pages to be allocated
 */
struct Page *alloc_pages(size_t n)
{
	struct Page *page;
	bool intr_flag;
try_again:
	local_intr_save(intr_flag);
	{
		page = pmm_manager->alloc_pages(n);
	}
	local_intr_restore(intr_flag);
	if (page == NULL && try_free_pages(n)) {
		goto try_again;
	}

	pls_write(used_pages, pls_read(used_pages) + n);
	return page;
}

/**
 * boot_alloc_page - allocate one page using pmm->alloc_pages(1) 
 * @return the kernel virtual address of this allocated page
 * note: this function is used to get the memory for PDT(Page Directory Table)&PT(Page Table)
 */
void *boot_alloc_page(void)
{
	struct Page *p = alloc_page();
	if (p == NULL) {
		panic("boot_alloc_page failed.\n");
	}
	return page2kva(p);
}

/**
 * free_pages - call pmm->free_pages to free a continuing n*PAGESIZE memory
 * @param base the first page to be freed
 * @param n number of pages to be freed
 */
void free_pages(struct Page *base, size_t n)
{
	bool intr_flag;
	local_intr_save(intr_flag);
	{
		pmm_manager->free_pages(base, n);
	}
	local_intr_restore(intr_flag);
	pls_write(used_pages, pls_read(used_pages) - n);
}

/**
 * nr_free_pages - call pmm->nr_free_pages to get the size (nr*PAGESIZE) of current free memory
 * @return number of free pages
 */
size_t nr_free_pages(void)
{
	size_t ret;
	bool intr_flag;
	local_intr_save(intr_flag);
	{
		ret = pmm_manager->nr_free_pages();
	}
	local_intr_restore(intr_flag);
	return ret;
}

/**************************************************
 * Memory tests
 **************************************************/
/**
 * Check the correctness of the system.
 */
void check_alloc_page(void)
{
	pmm_manager->check();

	kprintf("check_alloc_page() succeeded.\n");
}

/**
 * Check page table
 */
void check_pgdir(void)
{
	assert(npage <= KMEMSIZE / PGSIZE);
	assert(boot_pgdir != NULL && (uint32_t) PGOFF(boot_pgdir) == 0);
	assert(get_page(boot_pgdir, TEST_PAGE, NULL) == NULL);

	struct Page *p1, *p2;
	p1 = alloc_page();
	assert(page_insert(boot_pgdir, p1, TEST_PAGE, 0) == 0);

	pte_t *ptep;
	assert((ptep = get_pte(boot_pgdir, TEST_PAGE, 0)) != NULL);
	assert(pa2page(*ptep) == p1);
	assert(page_ref(p1) == 1);

	ptep = &((pte_t *) KADDR(PTE_ADDR(boot_pgdir[PDX(TEST_PAGE)])))[1];
	assert(get_pte(boot_pgdir, TEST_PAGE + PGSIZE, 0) == ptep);

	p2 = alloc_page();
	assert(page_insert(boot_pgdir, p2, TEST_PAGE + PGSIZE, PTE_U | PTE_W) ==
	       0);
	assert((ptep = get_pte(boot_pgdir, TEST_PAGE + PGSIZE, 0)) != NULL);
	assert(*ptep & PTE_U);
	assert(*ptep & PTE_W);
	assert(boot_pgdir[PDX(TEST_PAGE)] & PTE_U);
	assert(page_ref(p2) == 1);

	assert(page_insert(boot_pgdir, p1, TEST_PAGE + PGSIZE, 0) == 0);
	assert(page_ref(p1) == 2);
	assert(page_ref(p2) == 0);
	assert((ptep = get_pte(boot_pgdir, TEST_PAGE + PGSIZE, 0)) != NULL);
	assert(pa2page(*ptep) == p1);
	assert((*ptep & PTE_U) == 0);

	page_remove(boot_pgdir, TEST_PAGE);
	assert(page_ref(p1) == 1);
	assert(page_ref(p2) == 0);

	page_remove(boot_pgdir, TEST_PAGE + PGSIZE);
	assert(page_ref(p1) == 0);
	assert(page_ref(p2) == 0);

	assert(page_ref(pa2page(boot_pgdir[PDX(TEST_PAGE)])) == 1);
	free_page(pa2page(boot_pgdir[PDX(TEST_PAGE)]));
	boot_pgdir[PDX(TEST_PAGE)] = 0;

	kprintf("check_pgdir() succeeded.\n");
}
