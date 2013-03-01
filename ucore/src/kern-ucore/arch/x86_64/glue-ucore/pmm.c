#include <pmm.h>
#include <slab.h>
#include <error.h>
#include <assert.h>
#include <atom.h>
#include <string.h>
#include <kio.h>
#include <swap.h>
#include <glue_mp.h>
#include <glue_intr.h>
#include <mp.h>

#define SV_GDT_SIZE (8 + LAPIC_COUNT)

/* *
 * Global Descriptor Table:
 *
 * The kernel and user segments are identical (except for the DPL). To load
 * the %ss register, the CPL must equal the DPL. Thus, we must duplicate the
 * segments for the user and the kernel. Defined as follows:
 *   - 0x0 :  unused (always faults -- for trapping NULL far pointers)
 *   - 0x10:  kernel code segment
 *   - 0x20:  kernel data segment
 *   - 0x30:  kernel pls segment
 *   - 0x40:  user code segment
 *   - 0x50:  user data segment
 *   - 0x60:  defined for tss, initialized in gdt_init
 * */
PLS struct segdesc pls_gdt[SV_GDT_SIZE];
PLS struct pseudodesc pls_gdt_pd;

static volatile size_t used_pages;
PLS list_entry_t pls_page_struct_free_list;

static struct Page *page_struct_alloc(uintptr_t pa)
{
	list_entry_t *page_struct_free_list =
	    pls_get_ptr(page_struct_free_list);

	if (list_empty(page_struct_free_list)) {
		struct Page *p = KADDR_DIRECT(kalloc_pages(1));

		int i;
		for (i = 0; i < PGSIZE / sizeof(struct Page); ++i)
			list_add(page_struct_free_list, &p[i].page_link);
	}

	list_entry_t *entry = list_next(page_struct_free_list);
	list_del(entry);

	struct Page *page = le2page(entry, page_link);

	page->pa = pa;
	atomic_set(&page->ref, 0);
	page->flags = 0;
	list_init(entry);

	return page;
}

static void page_struct_free(struct Page *page)
{
	list_entry_t *page_struct_free_list =
	    pls_get_ptr(page_struct_free_list);
	list_add(page_struct_free_list, &page->page_link);
}

struct Page *alloc_pages(size_t npages)
{
	struct Page *result;
	uintptr_t base = kalloc_pages(npages);
	size_t i;
	int flags;

	local_intr_save_hw(flags);

	result = page_struct_alloc(base);
	kpage_private_set(base, result);

	for (i = 1; i < npages; ++i) {
		struct Page *page = page_struct_alloc(base + i * PGSIZE);
		kpage_private_set(base + i * PGSIZE, page);
	}

	while (1) {
		size_t old = used_pages;
		if (cmpxchg64(&used_pages, old, old + npages) == old)
			break;
	}

	local_intr_restore_hw(flags);
	return result;
}

void free_pages(struct Page *base, size_t npages)
{
	uintptr_t basepa = page2pa(base);
	size_t i;
	int flags;

	local_intr_save_hw(flags);

	for (i = 0; i < npages; ++i) {
		page_struct_free(kpage_private_get(basepa + i * PGSIZE));
	}

	kfree_pages(basepa, npages);
	while (1) {
		size_t old = used_pages;
		if (cmpxchg64(&used_pages, old, old - npages) == old)
			break;
	}

	local_intr_restore_hw(flags);
}

size_t nr_used_pages(void)
{
	return used_pages;
}

/* Copy from supervisor/arch/x86_64/mm/pmm.c */
static inline void lgdt(struct pseudodesc *pd)
{
	asm volatile ("lgdt (%0)"::"r" (pd));
	asm volatile ("movw %%ax, %%es"::"a" (KERNEL_DS));
	asm volatile ("movw %%ax, %%ds"::"a" (KERNEL_DS));
	// reload cs & ss
	asm volatile ("movq %%rsp, %%rax;"	// move %rsp to %rax
		      "pushq %1;"	// push %ss
		      "pushq %%rax;"	// push %rsp
		      "pushfq;"	// push %rflags
		      "pushq %0;"	// push %cs
		      "call 1f;"	// push %rip
		      "jmp 2f;"
		      "1: iretq; 2:"::"i" (KERNEL_CS), "i"(KERNEL_DS));
}

void pmm_init(void)
{
	used_pages = 0;
}

void pmm_init_ap(void)
{
	list_entry_t *page_struct_free_list =
	    pls_get_ptr(page_struct_free_list);
	list_init(page_struct_free_list);

	struct pseudodesc *gdt_pd = pls_get_ptr(gdt_pd);
	struct segdesc *gdt = pls_get_ptr(gdt);
	struct segdesc *sv_gdt = get_sv_gdt();
	memcpy(gdt, sv_gdt, SV_GDT_SIZE * sizeof(struct segdesc));
	gdt_pd->pd_lim = SV_GDT_SIZE * sizeof(struct segdesc) - 1;
	gdt_pd->pd_base = (uintptr_t) gdt;
	lgdt(gdt_pd);
}
