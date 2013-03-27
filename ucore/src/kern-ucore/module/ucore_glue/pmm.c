/*
 * =====================================================================================
 *
 *       Filename:  pmm_glue.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/21/2012 03:37:47 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#include <pmm.h>
#include <pmm_glue.h>
#include <vmm.h>
#include <proc.h>
#include <assert.h>

void *ucore_kva_alloc_pages(size_t n, unsigned int flags)
{
	struct Page *pages = alloc_pages(n);
	if (!pages)
		return NULL;
	if (flags & UCORE_KAP_IO) {
		int i;
		for (i = 0; i < n; i++)
			SetPageIO(&pages[i]);
	}
	return page2kva(pages);
}

void *ucore_map_pfn_range(unsigned long addr, unsigned long pfn,
			  unsigned long size, unsigned long flags)
{
	void *ret = NULL;
	struct mm_struct *mm = current->mm;
	uint32_t vm_flags = VM_READ | VM_WRITE;
	if (flags & VM_IO)
		vm_flags |= VM_IO;
	pte_perm_t perm = PTE_P | PTE_U | PTE_W;
	assert(mm);
	lock_mm(mm);
	if (addr == 0) {
		if ((addr = get_unmapped_area(mm, size)) == 0) {
			goto out_unlock;
		}
	}
	if (mm_map(mm, addr, size, vm_flags, NULL) == 0) {
		ret = (void *)addr;
	}
	pfn = pfn << PGSHIFT;
	size = (size + PGSIZE - 1) / PGSIZE;
	for (; size > 0; size--, pfn += PGSIZE, addr += PGSIZE)
		page_insert(mm->pgdir, pa2page(pfn), addr, perm);
out_unlock:
	unlock_mm(mm);
	return ret;
}

/* defined in asm-generic/memory_model.h, modified */
void *__pfn_to_page(size_t pfn)
{
	return (void *)&pages[pfn];
}

size_t __page_to_pfn(void *pg)
{
	return page2ppn((struct Page *)pg);
}
