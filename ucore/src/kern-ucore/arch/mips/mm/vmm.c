#include <arch.h>
#include <vmm.h>
#include <sync.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <error.h>
#include <slab.h>
#include <pmm.h>
#include <thumips_tlb.h>

/* 
   vmm design include two parts: mm_struct (mm) & vma_struct (vma)
   mm is the memory manager for the set of continuous virtual memory  
   area which have the same PDT. vma is a continuous virtual memory area.
   There a linear link list for vma & a redblack link list for vma in mm.
   ---------------
   mm related functions:
   golbal functions
   struct mm_struct * mm_create(void)
   void mm_destroy(struct mm_struct *mm)
   int do_pgfault(struct mm_struct *mm, uint32_t error_code, uintptr_t addr)
   --------------
   vma related functions:
   global functions
   struct vma_struct * vma_create (uintptr_t vm_start, uintptr_t vm_end,...)
   void insert_vma_struct(struct mm_struct *mm, struct vma_struct *vma)
   struct vma_struct * find_vma(struct mm_struct *mm, uintptr_t addr)
   local functions
   inline void check_vma_overlap(struct vma_struct *prev, struct vma_struct *next)
   ---------------
   check correctness functions
   void check_vmm(void);
   void check_vma_struct(void);
   void check_pgfault(void);
   */

static void check_vmm(void);
static void check_vma_struct(void);
static void check_pgfault(void);

int swap_init_ok = 0;

// check_vma_overlap - check if vma1 overlaps vma2 ?
static inline void
check_vma_overlap(struct vma_struct *prev, struct vma_struct *next)
{
	assert(prev->vm_start < prev->vm_end);
	assert(prev->vm_end <= next->vm_start);
	assert(next->vm_start < next->vm_end);
}

bool
copy_from_user(struct mm_struct *mm, void *dst, const void *src, size_t len,
	       bool writable)
{
	if (!user_mem_check(mm, (uintptr_t) src, len, writable)) {
		return 0;
	}
	memcpy(dst, src, len);
	return 1;
}

bool copy_to_user(struct mm_struct * mm, void *dst, const void *src, size_t len)
{
	if (!user_mem_check(mm, (uintptr_t) dst, len, 1)) {
		return 0;
	}
	memcpy(dst, src, len);
	return 1;
}

// check_vmm - check correctness of vmm
static void check_vmm(void)
{
	size_t nr_free_pages_store = nr_free_pages();

	check_vma_struct();
	check_pgfault();

	assert(nr_free_pages_store == nr_free_pages());

	kprintf("check_vmm() succeeded.\n");
}

static void check_vma_struct(void)
{
	size_t nr_free_pages_store = nr_free_pages();

	struct mm_struct *mm = mm_create();
	assert(mm != NULL);

	int step1 = 10, step2 = step1 * 10;

	int i;
	for (i = step1; i >= 0; i--) {
		struct vma_struct *vma = vma_create(i * 5, i * 5 + 2, 0);
		assert(vma != NULL);
		insert_vma_struct(mm, vma);
	}

	for (i = step1 + 1; i <= step2; i++) {
		struct vma_struct *vma = vma_create(i * 5, i * 5 + 2, 0);
		assert(vma != NULL);
		insert_vma_struct(mm, vma);
	}

	list_entry_t *le = list_next(&(mm->mmap_list));

	for (i = 0; i <= step2; i++) {
		assert(le != &(mm->mmap_list));
		struct vma_struct *mmap = le2vma(le, list_link);
		assert(mmap->vm_start == i * 5 && mmap->vm_end == i * 5 + 2);
		le = list_next(le);
	}

	for (i = 0; i < 5 * step2 + 2; i++) {
		struct vma_struct *vma = find_vma(mm, i);
		assert(vma != NULL);
		int j = __divu5(i);
		if (i >= 5 * j + 2) {
			j++;
		}
		assert(vma->vm_start == j * 5 && vma->vm_end == j * 5 + 2);
	}

	mm_destroy(mm);

	assert(nr_free_pages_store == nr_free_pages());

	kprintf("check_vma_struct() succeeded!\n");
}

struct mm_struct *check_mm_struct;

// check_pgfault - check correctness of pgfault handler
static void check_pgfault(void)
{
	size_t nr_free_pages_store = nr_free_pages();

	check_mm_struct = mm_create();
	assert(check_mm_struct != NULL);

	struct mm_struct *mm = check_mm_struct;
	pde_t *pgdir = mm->pgdir = boot_pgdir;
	assert(pgdir[0] == 0);

	struct vma_struct *vma = vma_create(0, PTSIZE, VM_WRITE);
	assert(vma != NULL);

	insert_vma_struct(mm, vma);

	uintptr_t addr = 0x100;
	assert(find_vma(mm, addr) == vma);

	int i, sum = 0;
	for (i = 0; i < 100; i++) {
		*(char *)(addr + i) = i;
		sum += i;
	}
	for (i = 0; i < 100; i++) {
		sum -= *(char *)(addr + i);
	}
	assert(sum == 0);

	page_remove(pgdir, ROUNDDOWN_2N(addr, PGSHIFT));
	free_page(pa2page(pgdir[0]));
	pgdir[0] = 0;

	mm->pgdir = NULL;
	mm_destroy(mm);
	check_mm_struct = NULL;

	assert(nr_free_pages_store == nr_free_pages());

	kprintf("check_pgfault() succeeded!\n");
}

bool copy_string(struct mm_struct *mm, char *dst, const char *src, size_t maxn)
{
	size_t alen, part =
	    ROUNDDOWN_2N((uintptr_t) src + PGSIZE, PGSHIFT) - (uintptr_t) src;
	while (1) {
		if (part > maxn) {
			part = maxn;
		}
		if (!user_mem_check(mm, (uintptr_t) src, part, 0)) {
			return 0;
		}
		if ((alen = strnlen(src, part)) < part) {
			memcpy(dst, src, alen + 1);
			return 1;
		}
		if (part == maxn) {
			return 0;
		}
		memcpy(dst, src, part);
		dst += part, src += part, maxn -= part;
		part = PGSIZE;
	}
}
