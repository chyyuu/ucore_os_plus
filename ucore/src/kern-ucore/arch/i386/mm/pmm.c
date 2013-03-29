#include <types.h>
#include <arch.h>
#include <stdio.h>
#include <string.h>
#include <mmu.h>
#include <memlayout.h>
#include <pmm.h>
#include <buddy_pmm.h>
#include <sync.h>
#include <slab.h>
#include <error.h>
#include <proc.h>
#include <kio.h>
#include <mp.h>

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
static struct taskstate ts = { 0 };

// physical address of boot-time page directory
uintptr_t boot_cr3;

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

/* *
 * Global Descriptor Table:
 *
 * The kernel and user segments are identical (except for the DPL). To load
 * the %ss register, the CPL must equal the DPL. Thus, we must duplicate the
 * segments for the user and the kernel. Defined as follows:
 *   - 0x0 :  unused (always faults -- for trapping NULL far pointers)
 *   - 0x8 :  kernel code segment
 *   - 0x10:  kernel data segment
 *   - 0x18:  user code segment
 *   - 0x20:  user data segment
 *   - 0x28:  defined for tss, initialized in gdt_init
 * */
static struct segdesc gdt[] = {
	SEG_NULL,
	[SEG_KTEXT] = SEG(STA_X | STA_R, 0x0, 0xFFFFFFFF, DPL_KERNEL),
	[SEG_KDATA] = SEG(STA_W, 0x0, 0xFFFFFFFF, DPL_KERNEL),
	[SEG_UTEXT] = SEG(STA_X | STA_R, 0x0, 0xFFFFFFFF, DPL_USER),
	[SEG_UDATA] = SEG(STA_W, 0x0, 0xFFFFFFFF, DPL_USER),
	[SEG_TSS] = SEG_NULL,
};

static struct pseudodesc gdt_pd = {
	sizeof(gdt) - 1, (uintptr_t) gdt
};

static DEFINE_PERCPU_NOINIT(size_t, used_pages);
DEFINE_PERCPU_NOINIT(list_entry_t, page_struct_free_list);

// virtual address of physicall page array
struct Page *pages;
// amount of physical memory (in pages)
size_t npage = 0;

// virtual address of boot-time page directory
pde_t *boot_pgdir = NULL;

// physical memory management
const struct pmm_manager *pmm_manager;

/* *
 * lgdt - load the global descriptor table register and reset the
 * data/code segement registers for kernel.
 * */
static inline void lgdt(struct pseudodesc *pd)
{
	asm volatile ("lgdt (%0)"::"r" (pd));
	asm volatile ("movw %%ax, %%gs"::"a" (USER_DS));
	asm volatile ("movw %%ax, %%fs"::"a" (USER_DS));
	asm volatile ("movw %%ax, %%es"::"a" (KERNEL_DS));
	asm volatile ("movw %%ax, %%ds"::"a" (KERNEL_DS));
	asm volatile ("movw %%ax, %%ss"::"a" (KERNEL_DS));
	// reload cs
	asm volatile ("ljmp %0, $1f\n 1:\n"::"i" (KERNEL_CS));
}

/**************************************************
 * Memory manager wrappers.
 **************************************************/

/**
 * Initialize mm tragedy.
 *     Just put it here to make it be static still.
 */
static void init_pmm_manager(void)
{
	pmm_manager = &buddy_pmm_manager;
	kprintf("memory managment: %s\n", pmm_manager->name);
	pmm_manager->init();
}

//init_memmap - call pmm->init_memmap to build Page struct for free memory  
static void init_memmap(struct Page *base, size_t n)
{
	pmm_manager->init_memmap(base, n);
}

static void check_alloc_page(void)
{
	pmm_manager->check();
	kprintf("check_alloc_page() succeeded!\n");
}

/* *
 * load_rsp0 - change the RSP0 in default task state segment,
 * so that we can use different kernel stack when we trap frame
 * user to kernel.
 * */
void load_rsp0(uintptr_t rsp0)
{
	ts.ts_esp0 = rsp0;
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

/* gdt_init - initialize the default GDT and TSS */
static void gdt_init(void)
{
	// set boot kernel stack and default SS0
	load_rsp0((uintptr_t) bootstacktop);
	ts.ts_ss0 = KERNEL_DS;

	// initialize the TSS filed of the gdt
	gdt[SEG_TSS] =
	    SEGTSS(STS_T32A, (uintptr_t) & ts, sizeof(ts), DPL_KERNEL);

	// reload all segment registers
	lgdt(&gdt_pd);

	// load the TSS
	ltr(GD_TSS);
}

/* pmm_init - initialize the physical memory management */
static void page_init(void)
{
	struct e820map *memmap = (struct e820map *)(0x8000 + KERNBASE);
	uint64_t maxpa = 0;

	kprintf("e820map:\n");
	int i;
	for (i = 0; i < memmap->nr_map; i++) {
		uint64_t begin = memmap->map[i].addr, end =
		    begin + memmap->map[i].size;
		kprintf("  memory: %08llx, [%08llx, %08llx], type = %d.\n",
			memmap->map[i].size, begin, end - 1,
			memmap->map[i].type);
		if (memmap->map[i].type == E820_ARM) {
			if (maxpa < end && begin < KMEMSIZE) {
				maxpa = end;
			}
		}
	}
	if (maxpa > KMEMSIZE) {
		maxpa = KMEMSIZE;
	}

	extern char end[];

	npage = maxpa / PGSIZE;
	pages = (struct Page *)ROUNDUP((void *)end, PGSIZE);

	for (i = 0; i < npage; i++) {
		SetPageReserved(pages + i);
	}

	uintptr_t freemem =
	    PADDR((uintptr_t) pages + sizeof(struct Page) * npage);

	for (i = 0; i < memmap->nr_map; i++) {
		uint64_t begin = memmap->map[i].addr, end =
		    begin + memmap->map[i].size;
		if (memmap->map[i].type == E820_ARM) {
			if (begin < freemem) {
				begin = freemem;
			}
			if (end > KMEMSIZE) {
				end = KMEMSIZE;
			}
			if (begin < end) {
				begin = ROUNDUP(begin, PGSIZE);
				end = ROUNDDOWN(end, PGSIZE);
				if (begin < end) {
					init_memmap(pa2page(begin),
						    (end - begin) / PGSIZE);
				}
			}
		}
	}
}

static void enable_paging(void)
{
	lcr3(boot_cr3);

	// turn on paging
	uint32_t cr0 = rcr0();
	cr0 |=
	    CR0_PE | CR0_PG | CR0_AM | CR0_WP | CR0_NE | CR0_TS | CR0_EM |
	    CR0_MP;
	cr0 &= ~(CR0_TS | CR0_EM);
	lcr0(cr0);
}

size_t nr_used_pages(void)
{
	return get_cpu_var(used_pages);
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
		ptep_map(ptep, pa);
		ptep_set_perm(ptep, perm);
	}
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

	//temporary map: 
	//virtual_addr 3G~3G+4M = linear_addr 0~4M = linear_addr 3G~3G+4M = phy_addr 0~4M   
	boot_pgdir[0] = boot_pgdir[PDX(KERNBASE)];
	boot_pgdir[1] = boot_pgdir[PDX(KERNBASE) + 1];

	enable_paging();

	//reload gdt(third time,the last time) to map all physical memory
	//virtual_addr 0~4G=liear_addr 0~4G
	//then set kernel stack(ss:esp) in TSS, setup TSS in gdt, load TSS
	gdt_init();

	//disable the map of virtual_addr 0~4M
	boot_pgdir[0] = boot_pgdir[1] = 0;

	//now the basic virtual memory map(see memalyout.h) is established.
	//check the correctness of the basic virtual memory map.
	check_boot_pgdir();

	print_pgdir(kprintf);

	slab_init();
}

void pmm_init_ap(void)
{
	list_entry_t *page_struct_free_list =
	    get_cpu_ptr(page_struct_free_list);
	list_init(page_struct_free_list);
	get_cpu_var(used_pages) = 0;
}

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

	get_cpu_var(used_pages) += n;
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
	get_cpu_var(used_pages) -= n;
}

// invalidate a TLB entry, but only if the page tables being
// edited are the ones currently in use by the processor.
void tlb_update(pde_t * pgdir, uintptr_t la)
{
	tlb_invalidate(pgdir, la);
}

void tlb_invalidate(pde_t * pgdir, uintptr_t la)
{
	if (rcr3() == PADDR(pgdir)) {
		invlpg((void *)la);
	}
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

/*
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

	pte_t *ptep, perm;
	assert((ptep = get_pte(boot_pgdir, TEST_PAGE, 0)) != NULL);
	assert(pa2page(*ptep) == p1);
	assert(page_ref(p1) == 1);

	ptep = &((pte_t *) KADDR(PTE_ADDR(boot_pgdir[PDX(TEST_PAGE)])))[1];
	assert(get_pte(boot_pgdir, TEST_PAGE + PGSIZE, 0) == ptep);

	p2 = alloc_page();
	ptep_unmap(&perm);
	ptep_set_u_read(&perm);
	ptep_set_u_write(&perm);
	assert(page_insert(boot_pgdir, p2, TEST_PAGE + PGSIZE, perm) == 0);
	assert((ptep = get_pte(boot_pgdir, TEST_PAGE + PGSIZE, 0)) != NULL);
	assert(ptep_u_read(ptep));
	assert(ptep_u_write(ptep));
	assert(ptep_u_read(&(boot_pgdir[PDX(TEST_PAGE)])));
	assert(page_ref(p2) == 1);

	assert(page_insert(boot_pgdir, p1, TEST_PAGE + PGSIZE, 0) == 0);
	assert(page_ref(p1) == 2);
	assert(page_ref(p2) == 0);
	assert((ptep = get_pte(boot_pgdir, TEST_PAGE + PGSIZE, 0)) != NULL);
	assert(pa2page(*ptep) == p1);
	assert(!ptep_u_read(ptep));

	page_remove(boot_pgdir, TEST_PAGE);
	assert(page_ref(p1) == 1);
	assert(page_ref(p2) == 0);

	page_remove(boot_pgdir, TEST_PAGE + PGSIZE);
	assert(page_ref(p1) == 0);
	assert(page_ref(p2) == 0);

	assert(page_ref(pa2page(boot_pgdir[PDX(TEST_PAGE)])) == 1);
	free_page(pa2page(boot_pgdir[PDX(TEST_PAGE)]));
	boot_pgdir[PDX(TEST_PAGE)] = 0;
	exit_range(boot_pgdir, TEST_PAGE, TEST_PAGE + PGSIZE);

	kprintf("check_pgdir() succeeded.\n");
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
