//#include <defs.h>
#include <kio.h>
#include <string.h>
#include <mmu.h>
#include <memlayout.h>
#include <pmm.h>
#include <buddy_pmm.h>
//#include <default_pmm.h>
#include <sync.h>
#include <error.h>
#include <vmm.h>
//#include <kmalloc.h>

#include <system.h>
#include <nios2.h>
#include <tlb.h>
#include <io.h>
#include <arch.h>

/* *
 * Task State Segment:
 *
 * The TSS may reside anywhere in memory. A special segment register called
 * the Task Register (TR) holds a segment selector that points a valid TSS
 * segment descriptor which resides in the GDT. Therefore, to use a TSS
 * the following must be done in function gdt_init:
 *   - create a TSS descriptor entry in GDT
 *   - add enough information to the TSS in memory as needed
 *   - load the TR register with a segment selector for that segment
 *
 * There are several fileds in TSS for specifying the new stack pointer when a
 * privilege level change happens. But only the fields SS0 and ESP0 are useful
 * in our os kernel.
 *
 * The field SS0 contains the stack segment selector for CPL = 0, and the ESP0
 * contains the new ESP value for CPL = 0. When an interrupt happens in protected
 * mode, the x86 CPU will look in the TSS for SS0 and ESP0 and load their value
 * into SS and ESP respectively.
 * */

PLS static size_t pls_used_pages;
PLS list_entry_t pls_page_struct_free_list;

// virtual address of physicall page array
struct Page *pages;
// amount of physical memory (in pages)
size_t npage = 0;

// virtual address of boot-time page directory
pde_t *boot_pgdir = NULL;
// physical address of boot-time page directory
uintptr_t boot_cr3;

// physical memory management
const struct pmm_manager *pmm_manager;

/* *
 * The page directory entry corresponding to the virtual address range
 * [VPT, VPT + PTSIZE) points to the page directory itself. Thus, the page
 * directory is treated as a page table as well as a page directory.
 *
 * One result of treating the page directory as a page table is that all PTEs
 * can be accessed though a "virtual page table" at virtual address VPT. And the
 * PTE for number n is stored in vpt[n].
 *
 * A second consequence is that the contents of the current page directory will
 * always available at virtual address PGADDR(PDX(VPT), PDX(VPT), 0), to which
 * vpd is set bellow.
 * */
pte_t *const vpt = (pte_t *) VPT;
pde_t *const vpd = (pde_t *) PGADDR(PDX(VPT), PDX(VPT), 0);

static void check_alloc_page(void);
static void check_pgdir(void);
static void check_boot_pgdir(void);

/* *
 * load_esp0 - change the ESP0 in default task state segment,
 * so that we can use different kernel stack when we trap frame
 * user to kernel.
 * */
void load_esp0(uintptr_t esp0)
{
	panic("pmm::load_esp0\n");
}

//init_pmm_manager - initialize a pmm_manager instance
static void init_pmm_manager(void)
{
	pmm_manager = &buddy_pmm_manager;
	//pmm_manager = &default_pmm_manager;
	kprintf("memory management: %s\n", pmm_manager->name);
	pmm_manager->init();
}

//init_memmap - call pmm->init_memmap to build Page struct for free memory  
static void init_memmap(struct Page *base, size_t n)
{
	pmm_manager->init_memmap(base, n);
}

//alloc_pages - call pmm->alloc_pages to allocate a continuous n*PAGESIZE memory 
struct Page *alloc_pages(size_t n)
{
	struct Page *page;
	int intr_flag;
	local_intr_save(intr_flag);
	{
		page = pmm_manager->alloc_pages(n);
	}
	local_intr_restore(intr_flag);
	return page;
}

//free_pages - call pmm->free_pages to free a continuous n*PAGESIZE memory 
void free_pages(struct Page *base, size_t n)
{
	int intr_flag;
	local_intr_save(intr_flag);
	{
		pmm_manager->free_pages(base, n);
	}
	local_intr_restore(intr_flag);
}

//nr_free_pages - call pmm->nr_free_pages to get the size (nr*PAGESIZE) 
//of current free memory
size_t nr_free_pages(void)
{
	size_t ret;
	int intr_flag;
	local_intr_save(intr_flag);
	{
		ret = pmm_manager->nr_free_pages();
	}
	local_intr_restore(intr_flag);
	return ret;
}

/* pmm_init - initialize the physical memory management */
static void page_init(void)
{
	int i;

	npage = NPAGES_SRAM + NPAGES_SDRAM;

	assert(npage);

	extern char end[];
	kprintf("page_init: end=0x%x\n", end);

	pages = (struct Page *)ROUNDUP((void *)end, PGSIZE);

	for (i = 0; i < npage; i++) {
		SetPageReserved(pages + i);
	}

	//init_memmap for sram
	{
		uintptr_t begin_sram = SRAM_BASE;
		uintptr_t end_sram = SRAM_END + 1;
		begin_sram = ROUNDUP(begin_sram, PGSIZE);
		end_sram = ROUNDDOWN(end_sram, PGSIZE);
		init_memmap(pa2page(begin_sram),
			    (end_sram - begin_sram) / PGSIZE);
	}

	//init_memmap for sdram
	{
		uintptr_t begin_sdram =
		    PADDR((uintptr_t) pages + sizeof(struct Page) * npage);
		uintptr_t end_sdram = SDRAM_END + 1;
		begin_sdram = ROUNDUP(begin_sdram, PGSIZE);
		end_sdram = ROUNDDOWN(end_sdram, PGSIZE);
		init_memmap(pa2page(begin_sdram),
			    (end_sdram - begin_sdram) / PGSIZE);
	}
}

static void enable_paging(void)
{
	lcr3(boot_cr3);
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

void load_rsp0(uintptr_t rsp0)
{
	//do nothing
}

/**
 * set_pgdir - save the physical address of the current pgdir
 */
void set_pgdir(struct proc_struct *proc, pde_t * pgdir)
{
	assert(proc != NULL);
	proc->cr3 = PADDR(pgdir);
}

/**
 * load_pgdir - use the page table specified in @proc by @cr3
 */
void load_pgdir(struct proc_struct *proc)
{
	if (proc != NULL)
		lcr3(proc->cr3);
	else
		lcr3(boot_cr3);
}

/**
 * map_pgdir - map the current pgdir @pgdir to its own address space
 */
void map_pgdir(pde_t * pgdir)
{
	pgdir[PDX(VPT)] = PADDR(pgdir) | PTE_P | PTE_W;
}

//boot_map_segment - setup&enable the paging mechanism
// parameters
//  la:   linear address of this memory need to map (after x86 segment map)
//  size: memory size
//  pa:   physical address of this memory
//  perm: permission of this memory  
static void
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

//boot_alloc_page - allocate one page using pmm->alloc_pages(1) 
// return value: the kernel virtual address of this allocated page
//note: this function is used to get the memory for PDT(Page Directory Table)&PT(Page Table)
static void *boot_alloc_page(void)
{
	struct Page *p = alloc_page();
	if (p == NULL) {
		panic("boot_alloc_page failed.\n");
	}
	return page2kva(p);
}

//pmm_init - setup a pmm to manage physical memory, build PDT&PT to setup paging mechanism 
//         - check the correctness of pmm & paging mechanism, print PDT&PT
void pmm_init(void)
{
	//We need to alloc/free the physical memory (granularity is 4KB or other size). 
	//So a framework of physical memory manager (struct pmm_manager)is defined in pmm.h
	//First we should init a physical memory manager(pmm) based on the framework.
	//Then pmm can alloc/free the physical memory. 
	//Now the first_fit/best_fit/worst_fit/buddy_system pmm are available.
	init_pmm_manager();

	// detect physical memory space, reserve already used memory,
	// then use pmm->init_memmap to create free page list
	page_init();

	//use pmm->check to verify the correctness of the alloc/free function in a pmm
	check_alloc_page();

	// create boot_pgdir, an initial page directory(Page Directory Table, PDT)
	boot_pgdir = boot_alloc_page();
	memset(boot_pgdir, 0, PGSIZE);

	boot_cr3 = PADDR(boot_pgdir);
	check_pgdir();

	static_assert(KERNBASE % PTSIZE == 0 && KERNTOP % PTSIZE == 0);

	// recursively insert boot_pgdir in itself
	// to form a virtual page table at virtual address VPT
	boot_pgdir[PDX(VPT)] = PADDR(boot_pgdir) | PTE_P | PTE_W;

	// map all physical memory to linear memory with base linear addr KERNBASE
	//linear_addr KERNBASE~KERNBASE+KMEMSIZE = phy_addr 0~KMEMSIZE
	//But shouldn't use this map until enable_paging() & gdt_init() finished.
	boot_map_segment(boot_pgdir, KERNBASE, KMEMSIZE, 0, PTE_W);

	enable_paging();

	//reload gdt(third time,the last time) to map all physical memory
	//virtual_addr 0~4G=liear_addr 0~4G
	//then set kernel stack(ss:esp) in TSS, setup TSS in gdt, load TSS
	//gdt_init();

	//disable the map of virtual_addr 0~4M
	boot_pgdir[0] = 0;

	//now the basic virtual memory map(see memalyout.h) is established.
	//check the correctness of the basic virtual memory map.

	check_boot_pgdir();

	print_pgdir(kprintf);

	slab_init();
	//panic("over\n");
}

//page_remove_pte - free an Page sturct which is related linear address la
//                - and clean(invalidate) pte which is related linear address la
//note: PT is changed, so the TLB need to be invalidate 
static inline void page_remove_pte(pde_t * pgdir, uintptr_t la, pte_t * ptep)
{
	/* LAB2 EXERCISE3: 2009011362
	 *
	 * please check if ptep is valid
	 * tlb must be manually updated if mapping is updated
	 */
	// YOU want comment, HERE is comment
	if (*ptep & PTE_P) {	// check if page directory is present
		struct Page *p = pte2page(*ptep);	// find corresponding page to pte
		int cnt = page_ref_dec(p);	// decrease page reference
		if (cnt == 0)
			free_pages(p, 1);	// and free it when reach 0
		*ptep = 0;	// clear page directory entry
		tlb_invalidate(pgdir, la);	// flush tlb
		//tlb_init();
	}
}

// invalidate a TLB entry, but only if the page tables being
// edited are the ones currently in use by the processor.
void tlb_invalidate(pde_t * pgdir, uintptr_t la)
{
	if (rcr3() == pgdir) {
		tlb_init();
	}
	/*
	   if (rcr3() == PADDR(pgdir)) {
	   invlpg((void *)la);
	   }
	 */
}

static void check_alloc_page(void)
{
	pmm_manager->check();
	kprintf("check_alloc_page() succeeded!\n");
}

static void check_pgdir(void)
{
	assert(npage <= KMEMSIZE / PGSIZE);
	assert(boot_pgdir != NULL && (uint32_t) PGOFF(boot_pgdir) == 0);

	assert(get_page(boot_pgdir, 0x0, NULL) == NULL);

	struct Page *p1, *p2;
	p1 = alloc_page();
	assert(page_insert(boot_pgdir, p1, 0x0, 0) == 0);

	pte_t *ptep;
	assert((ptep = get_pte(boot_pgdir, 0x0, 0)) != NULL);
	assert(pa2page(*ptep) == p1);
	assert(page_ref(p1) == 1);

	ptep = &((pte_t *) KADDR(PDE_ADDR(boot_pgdir[0])))[1];
	assert(get_pte(boot_pgdir, PGSIZE, 0) == ptep);

	p2 = alloc_page();
	assert(page_insert(boot_pgdir, p2, PGSIZE, PTE_U | PTE_W) == 0);
	assert((ptep = get_pte(boot_pgdir, PGSIZE, 0)) != NULL);
	assert(*ptep & PTE_U);
	assert(*ptep & PTE_W);
	assert(boot_pgdir[0] & PTE_U);
	assert(page_ref(p2) == 1);

	assert(page_insert(boot_pgdir, p1, PGSIZE, 0) == 0);
	assert(page_ref(p1) == 2);
	assert(page_ref(p2) == 0);
	assert((ptep = get_pte(boot_pgdir, PGSIZE, 0)) != NULL);
	assert(pa2page(*ptep) == p1);
	assert((*ptep & PTE_U) == 0);

	page_remove(boot_pgdir, 0x0);
	assert(page_ref(p1) == 1);
	assert(page_ref(p2) == 0);

	page_remove(boot_pgdir, PGSIZE);
	assert(page_ref(p1) == 0);
	assert(page_ref(p2) == 0);

	assert(page_ref(pa2page(boot_pgdir[0])) == 1);
	free_page(pa2page(boot_pgdir[0]));
	boot_pgdir[0] = 0;

	kprintf("check_pgdir() succeeded!\n");
}

static void check_boot_pgdir(void)
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
	assert(page_insert(boot_pgdir, p, 0x100, PTE_W) == 0);
	assert(page_ref(p) == 1);
	assert(page_insert(boot_pgdir, p, 0x100 + PGSIZE, PTE_W) == 0);
	assert(page_ref(p) == 2);

	const char *str = "ucore: Hello world!!";
	strcpy((void *)0x100, str);
	assert(strcmp((void *)0x100, (void *)(0x100 + PGSIZE)) == 0);

	*(char *)(page2kva(p) + 0x100) = '\0';
	assert(strlen((const char *)0x100) == 0);

	free_page(p);
	free_page(pa2page(PDE_ADDR(boot_pgdir[0])));
	boot_pgdir[0] = 0;

	kprintf("check_boot_pgdir() succeeded!\n");
}

//perm2str - use string 'u,r,w,-' to present the permission
static const char *perm2str(int perm)
{
	static char str[4];
	str[0] = (perm & PTE_U) ? 'u' : '-';
	str[1] = 'r';
	str[2] = (perm & PTE_W) ? 'w' : '-';
	str[3] = '\0';
	return str;
}

//get_pgtable_items - In [left, right] range of PDT or PT, find a continuous linear addr space
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
		int perm = (table[start++] & PTE_USER);
		while (start < right && (table[start] & PTE_USER) == perm) {
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
	printf("-------------------- BEGIN --------------------\n");
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
	printf("--------------------- END ---------------------\n");
}

inline struct Page *NEXT_PAGE(struct Page *pg)
{
#if SDRAM_BASE < SRAM_BASE
	panic("SDRAM_BASE < SRAM_BASE : not supported!\n");
	//懒得填了
#else
	int pa = page2pa(pg);
	if (pa <= SRAM_END && pa + PGSIZE > SRAM_END) {
		//kprintf("NEXT_PAGE : pg=0x%x pa=0x%x\n", pg, pa);
		return pa2page(SDRAM_BASE);
	} else {
		return pa2page(pa + PGSIZE);
	}
#endif
}
