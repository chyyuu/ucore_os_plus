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
pgd_t *boot_pgdir = NULL;

// physical memory management
const struct pmm_manager *pmm_manager;

/**
 * call pmm->init_memmap to build Page struct for free memory  
 */
void init_memmap(struct Page *base, size_t n)
{
	pmm_manager->init_memmap(base, n);
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
#ifdef UCONFIG_SWAP
try_again:
#endif
	local_intr_save(intr_flag);
	{
		page = pmm_manager->alloc_pages(n);
	}
	local_intr_restore(intr_flag);
#ifdef UCONFIG_SWAP
	if (page == NULL && try_free_pages(n)) {
		goto try_again;
	}
#endif

	pls_write(used_pages, pls_read(used_pages) + n);
	return page;
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
