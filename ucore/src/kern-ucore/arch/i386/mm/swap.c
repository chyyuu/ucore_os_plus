#include <pmm.h>
#include <vmm.h>
#include <swap.h>
#include <swapfs.h>
#include <slab.h>
#include <assert.h>
#include <stdio.h>
#include <vmm.h>
#include <error.h>
#include <atomic.h>
#include <sync.h>
#include <string.h>
#include <stdlib.h>
#include <kio.h>

#ifdef UCONFIG_SWAP

typedef struct {
	list_entry_t swap_list;
	size_t nr_pages;
} swap_list_t;

extern unsigned short *mem_map;
extern swap_list_t active_list;
extern swap_list_t inactive_list;

#define nr_active_pages                 (active_list.nr_pages)
#define nr_inactive_pages               (inactive_list.nr_pages)

#define SWAP_UNUSED                     0xFFFF

int page_launder(void);
void refill_inactive_scan(void);
int swap_out_mm(struct mm_struct *mm, size_t require);

void check_mm_swap(void)
{
	size_t nr_free_pages_store = nr_free_pages();
	size_t slab_allocated_store = slab_allocated();

	int ret, i, j;
	for (i = 0; i < max_swap_offset; i++) {
		assert(mem_map[i] == SWAP_UNUSED);
	}

	extern struct mm_struct *check_mm_struct;
	assert(check_mm_struct == NULL);

	// step1: check mm_map

	struct mm_struct *mm0 = mm_create(), *mm1;
	assert(mm0 != NULL && list_empty(&(mm0->mmap_list)));

	uintptr_t addr0, addr1;

	addr0 = 0;
	do {
		ret = mm_map(mm0, addr0, PTSIZE, 0, NULL);
		assert(ret ==
		       (USER_ACCESS(addr0, addr0 + PTSIZE)) ? 0 : -E_INVAL);
		addr0 += PTSIZE;
	} while (addr0 != 0);

	addr0 = 0;
	for (i = 0; i < 1024; i++, addr0 += PTSIZE) {
		ret = mm_map(mm0, addr0, PGSIZE, 0, NULL);
		assert(ret == -E_INVAL);
	}

	mm_destroy(mm0);

	mm0 = mm_create();
	assert(mm0 != NULL && list_empty(&(mm0->mmap_list)));

	addr0 = 0, i = 0;
	do {
		ret = mm_map(mm0, addr0, PTSIZE - PGSIZE, 0, NULL);
		assert(ret ==
		       (USER_ACCESS(addr0, addr0 + PTSIZE)) ? 0 : -E_INVAL);
		if (ret == 0) {
			i++;
		}
		addr0 += PTSIZE;
	} while (addr0 != 0);

	addr0 = 0, j = 0;
	do {
		addr0 += PTSIZE - PGSIZE;
		ret = mm_map(mm0, addr0, PGSIZE, 0, NULL);
		assert(ret ==
		       (USER_ACCESS(addr0, addr0 + PGSIZE)) ? 0 : -E_INVAL);
		if (ret == 0) {
			j++;
		}
		addr0 += PGSIZE;
	} while (addr0 != 0);

	assert(j + 1 >= i);

	mm_destroy(mm0);

	assert(nr_free_pages_store == nr_free_pages());
	assert(slab_allocated_store == slab_allocated());

	kprintf("check_mm_swap: step1, mm_map ok.\n");

	// step2: check page fault

	mm0 = mm_create();
	assert(mm0 != NULL && list_empty(&(mm0->mmap_list)));

	// setup page table

	struct Page *page = alloc_page();
	assert(page != NULL);
	pde_t *pgdir = page2kva(page);
	memcpy(pgdir, boot_pgdir, PGSIZE);
	pgdir[PDX(VPT)] = PADDR(pgdir) | PTE_P | PTE_W;

	// prepare for page fault

	mm0->pgdir = pgdir;
	check_mm_struct = mm0;
	lcr3(PADDR(mm0->pgdir));

	uint32_t vm_flags = VM_WRITE | VM_READ;
	struct vma_struct *vma;

	addr0 = 0;
	do {
		if ((ret = mm_map(mm0, addr0, PTSIZE, vm_flags, &vma)) == 0) {
			break;
		}
		addr0 += PTSIZE;
	} while (addr0 != 0);

	assert(ret == 0 && addr0 != 0 && mm0->map_count == 1);
	assert(vma->vm_start == addr0 && vma->vm_end == addr0 + PTSIZE);

	// check pte entry

	pte_t *ptep;
	for (addr1 = addr0; addr1 < addr0 + PTSIZE; addr1 += PGSIZE) {
		ptep = get_pte(pgdir, addr1, 0);
		assert(ptep == NULL);
	}

	memset((void *)addr0, 0xEF, PGSIZE * 2);
	ptep = get_pte(pgdir, addr0, 0);
	assert(ptep != NULL && (*ptep & PTE_P));
	ptep = get_pte(pgdir, addr0 + PGSIZE, 0);
	assert(ptep != NULL && (*ptep & PTE_P));

	ret = mm_unmap(mm0, -PTSIZE, PTSIZE);
	assert(ret == -E_INVAL);
	ret = mm_unmap(mm0, addr0 + PTSIZE, PGSIZE);
	assert(ret == 0);

	addr1 = addr0 + PTSIZE / 2;
	ret = mm_unmap(mm0, addr1, PGSIZE);
	assert(ret == 0 && mm0->map_count == 2);

	ret = mm_unmap(mm0, addr1 + 2 * PGSIZE, PGSIZE * 4);
	assert(ret == 0 && mm0->map_count == 3);

	ret = mm_map(mm0, addr1, PGSIZE * 6, 0, NULL);
	assert(ret == -E_INVAL);
	ret = mm_map(mm0, addr1, PGSIZE, 0, NULL);
	assert(ret == 0 && mm0->map_count == 4);
	ret = mm_map(mm0, addr1 + 2 * PGSIZE, PGSIZE * 4, 0, NULL);
	assert(ret == 0 && mm0->map_count == 5);

	ret = mm_unmap(mm0, addr1 + PGSIZE / 2, PTSIZE / 2 - PGSIZE);
	assert(ret == 0 && mm0->map_count == 1);

	addr1 = addr0 + PGSIZE;
	for (i = 0; i < PGSIZE; i++) {
		assert(*(char *)(addr1 + i) == (char)0xEF);
	}

	ret = mm_unmap(mm0, addr1 + PGSIZE / 2, PGSIZE / 4);
	assert(ret == 0 && mm0->map_count == 2);
	ptep = get_pte(pgdir, addr0, 0);
	assert(ptep != NULL && (*ptep & PTE_P));
	ptep = get_pte(pgdir, addr0 + PGSIZE, 0);
	assert(ptep != NULL && *ptep == 0);

	ret = mm_map(mm0, addr1, PGSIZE, vm_flags, NULL);
	memset((void *)addr1, 0x88, PGSIZE);
	assert(*(char *)addr1 == (char)0x88 && mm0->map_count == 3);

	for (i = 1; i < 16; i += 2) {
		ret = mm_unmap(mm0, addr0 + PGSIZE * i, PGSIZE);
		assert(ret == 0);
		if (i < 8) {
			ret = mm_map(mm0, addr0 + PGSIZE * i, PGSIZE, 0, NULL);
			assert(ret == 0);
		}
	}
	assert(mm0->map_count == 13);

	ret = mm_unmap(mm0, addr0 + PGSIZE * 2, PTSIZE - PGSIZE * 2);
	assert(ret == 0 && mm0->map_count == 2);

	ret = mm_unmap(mm0, addr0, PGSIZE * 2);
	assert(ret == 0 && mm0->map_count == 0);

	kprintf("check_mm_swap: step2, mm_unmap ok.\n");

	// step3: check exit_mmap

	ret = mm_map(mm0, addr0, PTSIZE, vm_flags, NULL);
	assert(ret == 0);

	for (i = 0, addr1 = addr0; i < 4; i++, addr1 += PGSIZE) {
		*(char *)addr1 = (char)0xFF;
	}

	exit_mmap(mm0);
	for (i = 0; i < PDX(KERNBASE); i++) {
		assert(pgdir[i] == 0);
	}

	kprintf("check_mm_swap: step3, exit_mmap ok.\n");

	// step4: check dup_mmap

	for (i = 0; i < max_swap_offset; i++) {
		assert(mem_map[i] == SWAP_UNUSED);
	}

	ret = mm_map(mm0, addr0, PTSIZE, vm_flags, NULL);
	assert(ret != 0);

	addr1 = addr0;
	for (i = 0; i < 4; i++, addr1 += PGSIZE) {
		*(char *)addr1 = (char)(i * i);
	}

	ret = 0;
	ret += swap_out_mm(mm0, 10);
	ret += swap_out_mm(mm0, 10);
	assert(ret == 4);

	for (; i < 8; i++, addr1 += PGSIZE) {
		*(char *)addr1 = (char)(i * i);
	}

	// setup mm1

	mm1 = mm_create();
	assert(mm1 != NULL);

	page = alloc_page();
	assert(page != NULL);
	pgdir = page2kva(page);
	memcpy(pgdir, boot_pgdir, PGSIZE);
	pgdir[PDX(VPT)] = PADDR(pgdir) | PTE_P | PTE_W;
	mm1->pgdir = pgdir;

	ret = dup_mmap(mm1, mm0);
	assert(ret == 0);

	// switch to mm1

	check_mm_struct = mm1;
	lcr3(PADDR(mm1->pgdir));

	addr1 = addr0;
	for (i = 0; i < 8; i++, addr1 += PGSIZE) {
		assert(*(char *)addr1 == (char)(i * i));
		*(char *)addr1 = (char)0x88;
	}

	// switch to mm0

	check_mm_struct = mm0;
	lcr3(PADDR(mm0->pgdir));

	addr1 = addr0;
	for (i = 0; i < 8; i++, addr1 += PGSIZE) {
		assert(*(char *)addr1 == (char)(i * i));
	}

	// switch to boot_cr3

	check_mm_struct = NULL;
	lcr3(boot_cr3);

	// free memory

	exit_mmap(mm0);
	exit_mmap(mm1);

	free_page(kva2page(mm0->pgdir));
	mm_destroy(mm0);
	free_page(kva2page(mm1->pgdir));
	mm_destroy(mm1);

	kprintf("check_mm_swap: step4, dup_mmap ok.\n");

	refill_inactive_scan();
	page_launder();
	for (i = 0; i < max_swap_offset; i++) {
		assert(mem_map[i] == SWAP_UNUSED);
	}

	assert(nr_free_pages_store == nr_free_pages());
	assert(slab_allocated_store == slab_allocated());

	kprintf("check_mm_swap() succeeded.\n");
}

void check_mm_shm_swap(void)
{
	size_t nr_free_pages_store = nr_free_pages();
	size_t slab_allocated_store = slab_allocated();

	int ret, i;
	for (i = 0; i < max_swap_offset; i++) {
		assert(mem_map[i] == SWAP_UNUSED);
	}

	extern struct mm_struct *check_mm_struct;
	assert(check_mm_struct == NULL);

	struct mm_struct *mm0 = mm_create(), *mm1;
	assert(mm0 != NULL && list_empty(&(mm0->mmap_list)));

	struct Page *page = alloc_page();
	assert(page != NULL);
	pde_t *pgdir = page2kva(page);
	memcpy(pgdir, boot_pgdir, PGSIZE);
	pgdir[PDX(VPT)] = PADDR(pgdir) | PTE_P | PTE_W;

	mm0->pgdir = pgdir;
	check_mm_struct = mm0;
	lcr3(PADDR(mm0->pgdir));

	uint32_t vm_flags = VM_WRITE | VM_READ;

	uintptr_t addr0, addr1;

	addr0 = 0;
	do {
		if ((ret = mm_map(mm0, addr0, PTSIZE * 4, vm_flags, NULL)) == 0) {
			break;
		}
		addr0 += PTSIZE;
	} while (addr0 != 0);

	assert(ret == 0 && addr0 != 0 && mm0->map_count == 1);

	ret = mm_unmap(mm0, addr0, PTSIZE * 4);
	assert(ret == 0 && mm0->map_count == 0);

	struct shmem_struct *shmem = shmem_create(PTSIZE * 2);
	assert(shmem != NULL && shmem_ref(shmem) == 0);

	// step1: check share memory

	struct vma_struct *vma;

	addr1 = addr0 + PTSIZE * 2;
	ret = mm_map_shmem(mm0, addr0, vm_flags, shmem, &vma);
	assert(ret == 0);
	assert((vma->vm_flags & VM_SHARE) && vma->shmem == shmem
	       && shmem_ref(shmem) == 1);
	ret = mm_map_shmem(mm0, addr1, vm_flags, shmem, &vma);
	assert(ret == 0);
	assert((vma->vm_flags & VM_SHARE) && vma->shmem == shmem
	       && shmem_ref(shmem) == 2);

	// page fault

	for (i = 0; i < 4; i++) {
		*(char *)(addr0 + i * PGSIZE) = (char)(i * i);
	}
	for (i = 0; i < 4; i++) {
		assert(*(char *)(addr1 + i * PGSIZE) == (char)(i * i));
	}

	for (i = 0; i < 4; i++) {
		*(char *)(addr1 + i * PGSIZE) = (char)(-i * i);
	}
	for (i = 0; i < 4; i++) {
		assert(*(char *)(addr1 + i * PGSIZE) == (char)(-i * i));
	}

	// check swap

	ret = swap_out_mm(mm0, 8) + swap_out_mm(mm0, 8);
	assert(ret == 8 && nr_active_pages == 4 && nr_inactive_pages == 0);

	refill_inactive_scan();
	assert(nr_active_pages == 0 && nr_inactive_pages == 4);

	// write & read again

	memset((void *)addr0, 0x77, PGSIZE);
	for (i = 0; i < PGSIZE; i++) {
		assert(*(char *)(addr1 + i) == (char)0x77);
	}

	// check unmap

	ret = mm_unmap(mm0, addr1, PGSIZE * 4);
	assert(ret == 0);

	addr0 += 4 * PGSIZE, addr1 += 4 * PGSIZE;
	*(char *)(addr0) = (char)(0xDC);
	assert(*(char *)(addr1) == (char)(0xDC));
	*(char *)(addr1 + PTSIZE) = (char)(0xDC);
	assert(*(char *)(addr0 + PTSIZE) == (char)(0xDC));

	kprintf("check_mm_shm_swap: step1, share memory ok.\n");

	// setup mm1

	mm1 = mm_create();
	assert(mm1 != NULL);

	page = alloc_page();
	assert(page != NULL);
	pgdir = page2kva(page);
	memcpy(pgdir, boot_pgdir, PGSIZE);
	pgdir[PDX(VPT)] = PADDR(pgdir) | PTE_P | PTE_W;
	mm1->pgdir = pgdir;

	ret = dup_mmap(mm1, mm0);
	assert(ret == 0 && shmem_ref(shmem) == 4);

	// switch to mm1

	check_mm_struct = mm1;
	lcr3(PADDR(mm1->pgdir));

	for (i = 0; i < 4; i++) {
		*(char *)(addr0 + i * PGSIZE) = (char)(0x57 + i);
	}
	for (i = 0; i < 4; i++) {
		assert(*(char *)(addr1 + i * PGSIZE) == (char)(0x57 + i));
	}

	check_mm_struct = mm0;
	lcr3(PADDR(mm0->pgdir));

	for (i = 0; i < 4; i++) {
		assert(*(char *)(addr0 + i * PGSIZE) == (char)(0x57 + i));
		assert(*(char *)(addr1 + i * PGSIZE) == (char)(0x57 + i));
	}

	swap_out_mm(mm1, 4);
	exit_mmap(mm1);

	free_page(kva2page(mm1->pgdir));
	mm_destroy(mm1);

	assert(shmem_ref(shmem) == 2);

	kprintf("check_mm_shm_swap: step2, dup_mmap ok.\n");

	// free memory

	check_mm_struct = NULL;
	lcr3(boot_cr3);

	exit_mmap(mm0);
	free_page(kva2page(mm0->pgdir));
	mm_destroy(mm0);

	refill_inactive_scan();
	page_launder();
	for (i = 0; i < max_swap_offset; i++) {
		assert(mem_map[i] == SWAP_UNUSED);
	}

	assert(nr_free_pages_store == nr_free_pages());
	assert(slab_allocated_store == slab_allocated());

	kprintf("check_mm_shm_swap() succeeded.\n");
}
#endif
