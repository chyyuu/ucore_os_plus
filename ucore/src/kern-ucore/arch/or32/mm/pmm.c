#include <pmm.h>
#include <mmu.h>
#include <types.h>
#include <stdio.h>
#include <buddy_pmm.h>
#include <memlayout.h>
#include <board.h>
#include <sync.h>
#include <slab.h>
#include <proc.h>
#include <string.h>
#include <kio.h>

pte_t *const vpt = (pte_t *) VPT;
pde_t *const vpd = (pde_t *) PGADDR(PDX(VPT), PDX(VPT), 0);;

/*! TODO: This variable is needed in kernel/proc.c. It can be defined anywhere. */
char *bootstack = (char *)0x0;

uintptr_t boot_pgdir_pa;
uintptr_t current_pgdir_pa;	// for exception handler

void tlb_invalidate_all()
{
	bool intr_flag;

	local_intr_save(intr_flag);
	int i;
	for (i = 0; i < NR_SETS; i++) {
		mtspr(SPR_DTLBMR_BASE(0) + i, 0);
		mtspr(SPR_ITLBMR_BASE(0) + i, 0);
		asm("l.nop;l.nop;");
	}
	local_intr_restore(intr_flag);
}

/* *
 * load_rsp0 - change the RSP0 in default task state segment,
 * so that we can use different kernel stack when we trap frame
 * user to kernel.
 * */
void load_rsp0(uintptr_t esp0)
{
}

/**
 * set_pgdir - save the physical address of the current pgdir
 */
void set_pgdir(struct proc_struct *proc, pde_t * pgdir)
{
	assert(proc != NULL);
	pgdir[PDX(VPT)] =
	    PADDR(pgdir) | PTE_P | PTE_SPR_R | PTE_SPR_W | PTE_A | PTE_D;
	proc->cr3 = PADDR(pgdir);
}

/**
 * load_pgdir - use the page table specified in @proc by @cr3
 */
void load_pgdir(struct proc_struct *proc)
{
	if (proc != NULL)
		current_pgdir_pa = proc->cr3;
	else
		current_pgdir_pa = boot_pgdir_pa;
	tlb_invalidate_all();
}

/**
 * map_pgdir - map the current pgdir @pgdir to its own address space
 */
void map_pgdir(pde_t * pgdir)
{
}

//init_pmm_manager - initialize a pmm_manager instance
static void init_pmm_manager(void)
{
	pmm_manager = &buddy_pmm_manager;
	kprintf("memory managment: %s\n", pmm_manager->name);
	pmm_manager->init();
}

static void page_init(void)
{
	int i;
	/* struct e820map *memmap = (struct e820map *)0x0; */
	/* uint32_t maxpa = 0; */

	/* kprintf("e820map:\n"); */
	/* int i; */
	/* for (i = 0; i < memmap->nr_map; i ++) { */
	/*     uint64_t begin = memmap->map[i].addr, end = begin + memmap->map[i].size; */
	/*     kprintf("  memory: %08llx, [%08llx, %08llx], type = %d.\n", */
	/*             memmap->map[i].size, begin, end - 1, memmap->map[i].type); */
	/*     if (memmap->map[i].type == E820_ARM) { */
	/*         if (maxpa < end && begin < KMEMSIZE) { */
	/*             maxpa = end; */
	/*         } */
	/*     } */
	/* } */
	/* if (maxpa > KMEMSIZE) { */
	/*     maxpa = KMEMSIZE; */
	/* } */

	extern char end[];

	npage = RAM_SIZE / PGSIZE;
	pages = (struct Page *)ROUNDUP((void *)end, PGSIZE);

	for (i = 0; i < npage; i++) {
		SetPageReserved(pages + i);
	}

	uintptr_t freemem =
	    PADDR((uintptr_t) pages + sizeof(struct Page) * npage);
	uint32_t free_begin = ROUNDUP(freemem, PGSIZE), free_end = RAM_SIZE;
	init_memmap(pa2page(free_begin), (free_end - free_begin) / PGSIZE);

	kprintf("free memory: [0x%x, 0x%x)\n", free_begin, free_end);

	/* for (i = 0; i < memmap->nr_map; i ++) { */
	/*     uint64_t begin = memmap->map[i].addr, end = begin + memmap->map[i].size; */
	/*     if (memmap->map[i].type == E820_ARM) { */
	/*         if (begin < freemem) { */
	/*             begin = freemem; */
	/*         } */
	/*         if (end > KMEMSIZE) { */
	/*             end = KMEMSIZE; */
	/*         } */
	/*         if (begin < end) { */
	/*             begin = ROUNDUP(begin, PGSIZE); */
	/*             end = ROUNDDOWN(end, PGSIZE); */
	/*             if (begin < end) { */
	/*                 init_memmap(pa2page(begin), (end - begin) / PGSIZE); */
	/*             } */
	/*         } */
	/*     } */
	/* } */
}

/**
 * Actually we'll modify the code at 0x900 and 0xA00 so that they'll jump to the new handler.
 */
static void enable_paging(void)
{
	/* Modify the exception handler code */
	extern unsigned long __post_boot_dtlb_miss;
	extern unsigned long __post_boot_itlb_miss;

	uint32_t *dtlb_handler = (uint32_t *) (0x908);
	uint32_t *itlb_handler = (uint32_t *) (0xa08);

	*dtlb_handler =
	    ((uint32_t) & __post_boot_dtlb_miss - (uint32_t) dtlb_handler) >> 2;
	*itlb_handler =
	    ((uint32_t) & __post_boot_itlb_miss - (uint32_t) itlb_handler) >> 2;

	/* Flush the whole TLB */
	tlb_invalidate_all();
}

//pmm_init - setup a pmm to manage physical memory, build PDT&PT to setup paging mechanism 
//         - check the correctness of pmm & paging mechanism, print PDT&PT
void pmm_init(void)
{
	init_pmm_manager();
	page_init();

#ifndef NOCHECK
	//check_alloc_page();
#endif

	boot_pgdir = boot_alloc_page();
	memset(boot_pgdir, 0, PGSIZE);
	boot_pgdir_pa = PADDR(boot_pgdir);
	current_pgdir_pa = boot_pgdir_pa;

#ifndef NOCHECK
	//check_pgdir ();
#endif

	static_assert(KERNBASE % PTSIZE == 0 && KERNTOP % PTSIZE == 0);

	boot_pgdir[PDX(VPT)] =
	    PADDR(boot_pgdir) | PTE_P | PTE_SPR_R | PTE_SPR_W | PTE_A | PTE_D;
	boot_map_segment(boot_pgdir, KERNBASE, RAM_SIZE, 0,
			 PTE_SPR_R | PTE_SPR_W | PTE_A | PTE_D);

	enable_paging();
#ifndef NOCHECK
	//check_boot_pgdir ();
#endif

	print_pgdir(kprintf);

	slab_init();
}

// invalidate a TLB entry, but only if the page tables being
// edited are the ones currently in use by the processor.
void tlb_update(pde_t * pgdir, uintptr_t la)
{
	uint32_t set = (la >> PGSHIFT) & (NR_SETS - 1);
	mtspr(SPR_DTLBMR_BASE(0) + set, 0);
	mtspr(SPR_ITLBMR_BASE(0) + set, 0);
	asm("l.nop;l.nop;");
}

void tlb_invalidate(pde_t * pgdir, uintptr_t la)
{
	uint32_t set = (la >> PGSHIFT) & (NR_SETS - 1);
	mtspr(SPR_DTLBMR_BASE(0) + set, 0);
	mtspr(SPR_ITLBMR_BASE(0) + set, 0);
	asm("l.nop;l.nop;");
}

void check_boot_pgdir(void)
{
	pte_t *ptep;
	int i;
	for (i = 0; i < npage; i += PGSIZE) {
		assert((ptep =
			get_pte(boot_pgdir, (uintptr_t) KADDR(i), 0)) != NULL);
		assert(PTE_ADDR(*ptep) == i);
	}

	assert(PDE_ADDR(boot_pgdir[PDX(VPT)]) == PADDR(boot_pgdir));

	assert(boot_pgdir[0] == 0);

	struct Page *p;
	p = alloc_page();
	assert(page_insert
	       (boot_pgdir, p, 0x100,
		PTE_SPR_W | PTE_SPR_R | PTE_A | PTE_D) == 0);
	assert(page_ref(p) == 1);
	assert(page_insert
	       (boot_pgdir, p, 0x100 + PGSIZE,
		PTE_SPR_W | PTE_SPR_R | PTE_A | PTE_D) == 0);
	assert(page_ref(p) == 2);

	const char *str = "ucore: Hello world!!";
	strcpy((void *)0x100, str);
	//pte_t *ptepp = get_pte (boot_pgdir, 0x0, 0);
	assert(strcmp((void *)0x100, (void *)(0x100 + PGSIZE)) == 0);

	*(char *)(page2kva(p) + 0x100) = '\0';
	assert(strlen((const char *)0x100) == 0);

	free_page(p);
	free_page(pa2page(PDE_ADDR(boot_pgdir[0])));
	boot_pgdir[0] = 0;

	kprintf("check_boot_pgdir() succeeded!\n");
}

//perm2str - use string 'E,R,W,r,w,-' to present the permission
static const char *perm2str(int perm)
{
	static char str[6];
	str[0] = (perm & PTE_E) ? 'E' : '-';
	str[1] = (perm & PTE_SPR_R) ? 'R' : '-';
	str[2] = (perm & PTE_SPR_W) ? 'W' : '-';
	str[3] = (perm & PTE_USER_R) ? 'r' : '-';
	str[4] = (perm & PTE_USER_W) ? 'w' : '-';
	str[5] = '\0';
	return str;
}

//get_pgtable_items - In [left, right] range of PDT or PT, find a continuing linear addr space
//                  - (left_store*X_SIZE~right_store*X_SIZE) for PDT or PT
//                  - X_SIZE=PTSIZE=4M, if PDT; X_SIZE=PGSIZE=4K, if PT
// paramemters:
//  left:        no use ???
//  right:       the high side of table's range
//  start:       the low side of table's range
//  table:       the beginning addr of table
//  left_store:  the pointer of the high side of table's next range
//  right_store: the pointer of the low side of table's next range
// return value: 0 - not a invalid item range, perm - a valid item range with perm permission 
static int
get_pgtable_items(size_t left, size_t right, size_t start, uintptr_t * table,
		  size_t * left_store, size_t * right_store)
{
	if (start >= right) {
		return 0;
	}
	while (start < right && !(table[start] & PTE_P)) {
		start++;
	}
	if (start < right) {
		if (left_store != NULL) {
			*left_store = start;
		}
		int perm = (table[start++] & PTE_PERM);
		while (start < right && (table[start] & PTE_PERM) == perm) {
			start++;
		}
		if (right_store != NULL) {
			*right_store = start;
		}
		return perm;
	}
	return 0;
}

//print_pgdir - print the PDT&PT
void print_pgdir(int (*printf) (const char *fmt, ...))
{
	printf("--------------------- BEGIN ---------------------\n");
	size_t left, right = 0, perm;
	while ((perm =
		get_pgtable_items(0, NPDEENTRY, right, vpd, &left,
				  &right)) != 0) {
		printf("PDE(%03x) %08x-%08x %08x %s\n", right - left,
		       left * PTSIZE, right * PTSIZE, (right - left) * PTSIZE,
		       perm2str(perm));
		size_t l, r = left * NPTEENTRY;
		while ((perm =
			get_pgtable_items(left * NPTEENTRY, right * NPTEENTRY,
					  r, vpt, &l, &r)) != 0) {
			printf("  |-- PTE(%05x) %08x-%08x %08x %s\n", r - l,
			       l * PGSIZE, r * PGSIZE, (r - l) * PGSIZE,
			       perm2str(perm));
		}
	}
	printf("---------------------- END ----------------------\n");
}
