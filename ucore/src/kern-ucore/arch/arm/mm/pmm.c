#include <types.h>
#include <kio.h>
#include <stdio.h>
#include <string.h>
#include <mmu.h>
#include <memlayout.h>
#include <proc.h>
#include <pmm.h>
#include <buddy_pmm.h>
#include <sync.h>
#include <slab.h>
#include <swap.h>
#include <error.h>
#include <ide.h>
#include <trap.h>
#include <mp.h>
#include <ramdisk.h>

uint32_t do_set_tls(struct user_tls_desc *tlsp)
{
	void *tp = (void *)tlsp;
	current->tls_pointer = tp;
	asm("mcr p15, 0, %0, c13, c0, 3"::"r"(tp));
	return 0;
}

/* machine dependent */
inline static void flushCache926(void)
{
	unsigned int c7format = 0;
	asm volatile (" MCR p15,0,%0,c7,c7,0"::"r" (c7format):"memory");	/* flush I&D-cache */
}

inline static flushDcache926(void)
{
	unsigned int c7format = 0;
	asm volatile (" MCR p15,0,%0,c7,c6,0"::"r" (c7format):"memory");	/* flush D-cache */
}

inline static void flushIcache926(void)
{
	unsigned int c7format = 0;
	asm volatile (" MCR p15,0,%0,c7,c5,0"::"r" (c7format):"memory");	/* flush D-cache */
}

static Pagetable masterPT = { 0, 0, 0, MASTER, 0 };

static struct memmap masterMemmap = { 1,
	{
	 {SDRAM0_START, SDRAM0_SIZE, MEMMAP_MEMORY}
	 }
};

// virtual address of physicall page array
struct Page *pages;
// amount of physical memory (in pages)
size_t npage = 0;
unsigned long max_pfn;

static DEFINE_PERCPU_NOINIT(size_t, used_pages);
DEFINE_PERCPU_NOINIT(list_entry_t, page_struct_free_list);

// virtual address of boot-time page directory
pde_t *boot_pgdir = NULL;
// physical address of boot-time page directory
uintptr_t boot_pgdir_pa;

// physical memory management
const struct pmm_manager *pmm_manager;

static void check_alloc_page(void);
static void check_pgdir(void);
static void check_boot_pgdir(void);

//init_pmm_manager - initialize a pmm_manager instance
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

size_t nr_used_pages(void)
{
	return get_cpu_var(used_pages);
}

void pmm_init_ap(void)
{
	list_entry_t *page_struct_free_list =
	    get_cpu_ptr(page_struct_free_list);
	list_init(page_struct_free_list);
	get_cpu_var(used_pages) = 0;
}

//alloc_pages - call pmm->alloc_pages to allocate a continuous n*PAGESIZE memory 
struct Page *alloc_pages(size_t n)
{
	bool intr_flag;
	struct Page *page;
	local_intr_save(intr_flag);
	{
		page = pmm_manager->alloc_pages(n);
	}
	local_intr_restore(intr_flag);
	if (page)
		get_cpu_var(used_pages) += n;
	return page;
}

//free_pages - call pmm->free_pages to free a continuous n*PAGESIZE memory 
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

//nr_free_pages - call pmm->nr_free_pages to get the size (nr*PAGESIZE) 
//of current free memory
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

// Determine the amount of memory (assumption that memory is contiguous)
// note: KMEMSIZE maximum amount of physical memory
/* pmm_init - initialize the physical memory management */
static void page_init(void)
{
	//struct regionmap * memmap = (struct regionmap *) (0x10000 + KERNBASE); // where the memory map is
	struct memmap *memmap = &masterMemmap;

	// physical memory upper bound (see memmap)
	uint32_t maxpa = 0;

	// print memory map and compute maxpa
	kprintf("memory map:\n");
	int i;
	for (i = 0; i < memmap->nr_map; i++) {
		uint32_t begin = memmap->map[i].addr, end =
		    begin + memmap->map[i].size;
		kprintf("  memory: 0x%08x, [0x%08x, 0x%08x], type = %d.\n",
			memmap->map[i].size, begin, end - 1,
			memmap->map[i].type);
		if (memmap->map[i].type == MEMMAP_MEMORY) {
			if (maxpa < end) {
				maxpa = end;
			}
		}
	}
	if (maxpa > KERNTOP) {
		maxpa = KERNTOP;	// KERNBASE + KMEMSIZE is physical memory upper limit
	}
	// end address of kernel
	extern char end[];

	// number of pages of the non-existing and existing physical memory (kernel + free)
	npage = maxpa / PGSIZE;
	max_pfn = npage;

	// put page structure table at the end of kernel
	pages = (struct Page *)ROUNDUP((void *)end, PGSIZE);
	//kprintf("maxpa: 0x%08x  npage: 0x%08x  pages: 0x%08x  end: 0x%08x\n", maxpa, npage, pages, end);

	for (i = 0; i < npage; i++) {	// trick to not consider non existing pages
		SetPageReserved(pages + i);
	}

	// start address of free memory
	// kernel code zone | page structure table zone | free memory zone
	// warning, stack IS in free memory zone!!!!
	// TODO add ramdisk
	uintptr_t freemem =
	    PADDR((uintptr_t) pages + sizeof(struct Page) * npage);
	kprintf("freemem start at: 0x%08x\n", freemem);

	// free memory block
	for (i = 0; i < memmap->nr_map; i++) {
		uint32_t begin = memmap->map[i].addr, end =
		    begin + memmap->map[i].size;
		if (memmap->map[i].type == MEMMAP_MEMORY) {
			if (begin < freemem) {
				begin = freemem;
			}
			if (end > KERNTOP) {
				end = KERNTOP;
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

//boot_map_segment - setup&enable the paging mechanism
// parameters
//  la:   linear address of this memory need to map
//  size: memory size
//  pa:   physical address of this memory
//  perm: permission of this memory  
static void
boot_map_segment(pde_t * pgdir, uintptr_t la, size_t size, uintptr_t pa,
		 uint32_t perm)
{
	if (size == 0)
		return;
	assert(PGOFF(la) == PGOFF(pa));
	size_t n = ROUNDUP(size + PGOFF(la), PGSIZE) / PGSIZE;
	la = ROUNDDOWN(la, PGSIZE);
	pa = ROUNDDOWN(pa, PGSIZE);
	for (; n > 0; n--, la += PGSIZE, pa += PGSIZE) {
		pte_t *ptep = get_pte(pgdir, la, 1);
		assert(ptep != NULL);
		ptep_map(ptep, pa);
		ptep_set_perm(ptep, PTE_P | perm);
	}
}

void __boot_map_iomem(uintptr_t la, size_t size, uintptr_t pa)
{
	//kprintf("mapping iomem 0x%08x to 0x%08x, size 0x%08x\n", pa, la,size);
	boot_map_segment(boot_pgdir, la, size, pa, PTE_W | PTE_IOMEM);
}

//boot_alloc_page - allocate pages for the PDT 
// return value: the kernel virtual address of this allocated page
//note: this function is used to get the memory for PDT(Page Directory Table)
// The is only one PDT of size 16kB, hence 4 pages. However, if table is not 
// on aligned to 16kb, there will be a bug. We allocated 6 pages
// to make sure we have the margin to align correctly the pdt.
static void *boot_alloc_page(void)
{
	struct Page *p = alloc_pages(8);
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
	kprintf("begin check!!!");
	check_alloc_page();
	kprintf("end check!!!");

	// create boot_pgdir, an initial page directory(Page Directory Table, PDT)
	boot_pgdir = boot_alloc_page();
	// Align the PDT to the next 16kb boundary, ARM requirement
	boot_pgdir = ROUNDUP(boot_pgdir, 4 * PGSIZE);

	memset(boot_pgdir, 0, 4 * PGSIZE);
	boot_pgdir_pa = PADDR(boot_pgdir);
	kprintf("boot_pgdir at: 0x%08x \n", boot_pgdir);

	// initialize master table / PDT
	// TODO replace by simpler function
	masterPT.ptAddress = masterPT.masterPtAddress = boot_pgdir_pa;
	mmu_init_pdt(&masterPT);

	check_pgdir();

	static_assert(KERNBASE % PTSIZE == 0 && KERNTOP % PTSIZE == 0);

	// recursively insert boot_pgdir in itself
	// to form a virtual page table at virtual address VPT
	// The trick, however, is that we have to initialize up 
	// to L2 tables, hence the use of get_pte to write 
	// the PDT entries of where boot_pgdir is.
	// PDT is 16kb, so we need to set 4 PTE
	// objective: get_pte(boot_pgdir, VPT, 1) == boot_pgdir;
	// Permission: Kernel writeable

	map_pgdir(boot_pgdir);

	// map fixed address segments
	//boot_map_segment(boot_pgdir, virtual, PGSIZE, physical, PTEX_W); // base location of vector table
	extern char __kernel_text_start[], __kernel_text_end[];
	//kprintf("## %08x %08x\n", __kernel_text_start, __kernel_text_end);
	boot_map_segment(boot_pgdir, KERNBASE, KMEMSIZE, PADDR(KERNBASE), PTE_W);	// fixed address
	/* kernel code readonly protection */
	boot_map_segment(boot_pgdir, (uintptr_t) __kernel_text_start,
			 __kernel_text_end - __kernel_text_start,
			 (uintptr_t) PADDR(__kernel_text_start), PTE_P);

	boot_map_segment(boot_pgdir, KIOBASE, KIOSIZE, KIOBASE, PTE_W | PTE_IOMEM);	// fixed address

	//Add PTE_W|PTE_U by whn09
	//PTE_W should be aborted later
	boot_map_segment(boot_pgdir, 0xFFFF0000, PGSIZE, SDRAM0_START, PTE_PWT | PTE_W | PTE_U);	// high location of vector table

#ifdef UCONFIG_HAVE_RAMDISK
	if (CHECK_INITRD_EXIST()) {
		boot_map_segment(boot_pgdir, DISK_FS_VBASE,
				 ROUNDUP(initrd_end - initrd_begin, PGSIZE),
				 (uintptr_t) PADDR(initrd_begin), PTE_W);
		kprintf("mapping initrd to 0x%08x\n", DISK_FS_VBASE);
	}
#endif
#ifdef HAS_NANDFLASH
	if (check_nandflash()) {
		boot_map_segment(boot_pgdir, NAND_FS_VBASE, 0x10000000,
				 AT91C_SMARTMEDIA_BASE, PTE_W | PTE_IOMEM);
		kprintf("mapping nandflash to 0x%08x\n", NAND_FS_VBASE);
	}
#endif
#ifdef HAS_SDS
	if (check_sds()) {
		boot_map_segment(boot_pgdir, SDS_VBASE, SDS_VSIZE,
				 AT91C_BASE_EBI2, PTE_W | PTE_IOMEM);
		kprintf("mapping sds to 0x%08x\n", SDS_VBASE);
	}
#endif
#ifdef HAS_SHARED_KERNMEM
	struct Page *kshm_page = alloc_pages(SHARED_KERNMEM_PAGES);
	void *kern_shm_pbase = page2kva(kshm_page);
	assert(kern_shm_pbase);
	boot_map_segment(boot_pgdir, SHARED_KERNMEM_VBASE,
			 SHARED_KERNMEM_PAGES * PGSIZE,
			 (uintptr_t) PADDR(kern_shm_pbase), PTE_W | PTE_U);
	kprintf("mapping kern_shm 0x%08x to 0x%08x, size: %d Pages\n",
		kern_shm_pbase, SHARED_KERNMEM_VBASE, SHARED_KERNMEM_PAGES);
	*(uint32_t *) kern_shm_pbase = 0;

	*(uint32_t *) kern_shm_pbase = 0x48594853;
	*(uint32_t *) (kern_shm_pbase + 4) = 0x04;
	*(uint32_t *) (kern_shm_pbase + 12) = 0x00555555;
	*(uint32_t *) (kern_shm_pbase + 512) = 0x43444546;
#endif
	// ramdisk for swap
	//boot_map_segment(boot_pgdir, RAMDISK_START, 0x10000000, 0x10000000, PTE_W); // fixed address
	// high kernel WIP
	//boot_map_segment(boot_pgdir, 0xC0000000, KMEMSIZE, KERNBASE, PTE_W); // relocation address
	print_pgdir(kprintf);

	/* Part 3 activating page tables */
	ttbSet((uint32_t) PADDR(boot_pgdir));

	/* enable cache and MMU */
	mmu_init();

	/* ioremap */
	board_init();

	kprintf("mmu enabled.\n");
	print_pgdir(kprintf);

	//now the basic virtual memory map(see memalyout.h) is established.
	//check the correctness of the basic virtual memory map.
	//XXX This is a buggy test!
	//check_boot_pgdir();

	//print_pgdir(kprintf);

	/* boot_pgdir now should be VDT */
	boot_pgdir = (pde_t *) VPT_BASE;

	slab_init();

	kprintf("slab_init() done\n");

	/*
	 * Add by whn09, map 0xffff0fa0!
	 */
	unsigned long vectors = 0xffff0000;	//CONFIG_VECTORS_BASE = 0xffff0000
	extern char __kuser_helper_start[], __kuser_helper_end[];
	//kprintf("## whn09 start %08x %08x\n", __kuser_helper_start, __kuser_helper_end);
	int kuser_sz = __kuser_helper_end - __kuser_helper_start;
	memcpy((void *)vectors + 0x1000 - kuser_sz, __kuser_helper_start,
	       kuser_sz);
	//kprintf("## whn09 end %08x %08x\n", __kuser_helper_start, __kuser_helper_end);
	pte_t *ptep = get_pte(boot_pgdir, 0XFFFF0000, 1);
	assert(ptep != NULL);
	//Fix me
	//the page 0xffff0000's permission should be 'PTE_P | ~PTE_W | PTE_PWT | PTE_U',
	//but the code didn't work even though the permission of that page is set as mentioned above.
	ptep_set_perm(ptep, PTE_P | ~PTE_W | PTE_PWT | PTE_U);
	//memcpy((void*)vectors + 0x1000 - kuser_sz, __kuser_helper_start, kuser_sz);
	/*
	 * Add end
	 */
}

// invalidate both TLB 
// (clean and flush, meaning we write the data back)
void tlb_invalidate(pde_t * pgdir, uintptr_t la)
{
	asm volatile ("mcr p15, 0, %0, c8, c5, 1"::"r" (la):"cc");
	asm volatile ("mcr p15, 0, %0, c8, c6, 1"::"r" (la):"cc");
}

void tlb_update(pgd_t * pgdir, uintptr_t la)
{
	tlb_invalidate(pgdir, la);
}

void tlb_invalidate_all()
{
	// tlb_invalidate(0,0);
	const int zero = 0;
	asm volatile ("MCR p15, 0, %0, c8, c5, 0;"	/* invalidate TLB */
		      "MCR p15, 0, %0, c8, c6, 0"	/* invalidate TLB */
		      ::"r" (zero):"cc");
}

#if 0
void tlb_clean_flush(pde_t * pgdir, uintptr_t la)
{
	uint32_t c8format = la & ~0x1F;	// ARM 920T dependant
	asm volatile ("MCR p15, 0, %0, c7, c14, 1"	/* clean/flush TLB */
		      ::"r" (c8format)
	    );
}
#endif

/* *
 * load_rsp0 - change the RSP0 in default task state segment,
 * so that we can use different kernel stack when we trap frame
 * user to kernel.
 * */
void load_rsp0(uintptr_t esp0)
{
	/* 
	 * no such equivalence in ARM
	 * This means that kernel routines share a predefined 
	 * stack for each mode
	 * */
}

/**
 * set_pgdir - save the physical address of the current pgdir
 */
void set_pgdir(struct proc_struct *proc, pde_t * pgdir)
{
	//assert(0);
	assert(((uint32_t) (pgdir) & 0x3fff) == 0);
	assert(proc != NULL);
	//pgdir[PDX(VPT)] = PADDR(pgdir) | PTE_P | PTE_SPR_R | PTE_SPR_W | PTE_A | PTE_D;
	//pgdir[PDX(VPT_BASE)] = PADDR(pgdir) | PTEX_P | PTEX_W ;
	proc->cr3 = PADDR(pgdir);
}

#if 0
/**
 * load_pgdir - use the page table specified in @proc by @cr3
 */
void load_pgdir(struct proc_struct *proc)
{
	panic("NOT IMP\n");
	tlb_invalidate_all();
}
#endif

/**
 * map_pgdir - map the current pgdir @pgdir to its own address space
 */
//pgdir is vaddr
void map_pgdir(pde_t * pgdir)
{
	//panic("NOT IMP\n");
	//TODO
	//check PDT 16k alignment
	assert(((uint32_t) (pgdir) & 0x3fff) == 0);
	int i = 0;
	for (; i < 4; i++) {
		pte_t *ptep = get_pte(pgdir, VPT_BASE + i * PGSIZE, 1);
		ptep_map(ptep, PADDR(pgdir) + i * PGSIZE);
		pte_perm_t perm = PTE_P | PTE_W | PTE_PWT;
		//ptep_set_s_write(ptep);
		ptep_set_perm(ptep, perm);
		//*ptep |= PTEX_PWT; //write through
		//tlb_clean_flush(boot_pgdir, (uint32_t) ptep); // clean cache, write
		ptep++;
	}
}

static void check_alloc_page(void)
{
#ifndef BYPASS_CHECK
	pmm_manager->check();
	kprintf("check_alloc_page() succeeded!\n");
#endif /* !__BYPASS_CHECK__ */
}

static void check_pgdir(void)
{
#ifndef BYPASS_CHECK
	// make any sense?
//	assert(npage - KERNBASE / PGSIZE <= KMEMSIZE / PGSIZE);	// pdt exists but is empty
	// pdt has a valid address format
	assert(boot_pgdir != NULL && (uint32_t) PGOFF(boot_pgdir) == 0);
	// initial address empty
	assert(get_page(boot_pgdir, 0x0, NULL) == NULL);

	// Create a page p1, register it, check it has been registered
	struct Page *p1, *p2;
	p1 = alloc_page();
	assert(page_insert(boot_pgdir, p1, 0x0, 0) == 0);

	pte_t *ptep;
	assert((ptep = get_pte(boot_pgdir, 0x0, 0)) != NULL);
	assert(pa2page(*ptep) == p1);
	assert(page_ref(p1) == 1);

	ptep = &((pte_t *) KADDR(PDE_ADDR(boot_pgdir[0])))[1];
	assert(get_pte(boot_pgdir, PGSIZE, 0) == ptep);

	// Create a page p2 with some rules, verify that the rules have been kept
	p2 = alloc_page();
	assert(page_insert(boot_pgdir, p2, PGSIZE, PTE_U | PTE_W) == 0);
	assert((ptep = get_pte(boot_pgdir, PGSIZE, 0)) != NULL);
	assert(*PTE_STATUS(ptep) & PTE_U);
	assert(*PTE_STATUS(ptep) & PTE_W);
	//assert(boot_pgdir[0] & PTEX_UW);
	assert(page_ref(p2) == 1);

	// Replace p2 for p1, check that p2 has been replaced, check the number of reference of p1
	assert(page_insert(boot_pgdir, p1, PGSIZE, 0) == 0);
	assert(page_ref(p1) == 2);
	assert(page_ref(p2) == 0);
	assert((ptep = get_pte(boot_pgdir, PGSIZE, 0)) != NULL);
	assert(pa2page(*ptep) == p1);
	assert((*PTE_STATUS(ptep) & PTEX_U) == 0);

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
#endif /* !__BYPASS_CHECK__ */
}

static void check_boot_pgdir(void)
{
#ifndef BYPASS_CHECK

	//~ pte_t *ptep;
	//~ int i;

	// this section is not valid
	//~ for (i = 0; i < npage*PGSIZE; i += PGSIZE) {
	//~ assert((ptep = get_pte(boot_pgdir, (uintptr_t)KADDR(i), 0)) != NULL);
	//~ assert(PTE_ADDR(*ptep) == i);
	//~ }

	// Check that the virtual page directory points to the page directory
	//~ assert(PDE_ADDR(boot_pgdir[PDX(VPT)]) == PADDR(boot_pgdir));
	/*kprintf("0x%08x\n", PADDR(boot_pgdir));
	   kprintf("0x%08x\n", PDX(VPT));
	   kprintf("0x%08x\n", boot_pgdir[PDX(VPT)]);
	   kprintf("0x%08x\n", (pte_t *)PDE_ADDR(boot_pgdir[PDX(VPT)])); */

	assert(PTE_ADDR(*((pte_t *) PDE_ADDR(boot_pgdir[PDX(VPT_BASE)]))) ==
	       PADDR(boot_pgdir));

//  assert(boot_pgdir[0] == 0);

	// map two regions to the same page
	// warning: not 0x100, it interferes with the EVT
	struct Page *p;
	p = alloc_page();
	assert(page_insert(boot_pgdir, p, 0x100, PTE_W) == 0);
	assert(page_ref(p) == 1);
	assert(page_insert(boot_pgdir, p, 0x100 + PGSIZE, PTE_W) == 0);
	assert(page_ref(p) == 2);

	// copy a string to the first region, we should see it in the second region
	const char *str = "ucore: Hello world!!";
	strcpy((void *)0x100, str);
	assert(strcmp((void *)0x100, (void *)(0x100 + PGSIZE)) == 0);

	// write directly in the page in memory (fixed address), 
	// we should see it in the first region
	*(char *)(page2kva(p) + 0x100) = '\0';
	assert(strlen((const char *)0x100) == 0);

	free_page(p);
	free_page(pa2page(PDE_ADDR(boot_pgdir[0])));
	boot_pgdir[0] = 0;

	kprintf("check_boot_pgdir() succeeded!\n");
#endif /* !__BYPASS_CHECK__ */
}

//perm2str - use string 'u,r,w,-' to present the permission
static const char *perm2str(int perm)
{
	static char str[4];
	str[0] = (perm & PTEX_U) ? 'u' : '-';
	str[1] = 'r';
	str[2] = (perm & PTEX_W) ? 'w' : '-';
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
//  mask:                a mask to get perm: 0x3FF for PDT (at least get domain), PTE_UW for PT
// return value: 0 - not a invalid item range, perm - a valid item range with perm permission 
static int
get_pgtable_items(size_t left, size_t right, size_t start, uintptr_t * table,
		  size_t * left_store, size_t * right_store, size_t mask)
{
	if (start >= right) {
		return 0;
	}
	// (0, NPDEENTRY, right, vpd, &left, &right)
	//kprintf("PDE(%03x) %08x-%08x %08x %s\n", right - left,
	//      left * PTSIZE, right * PTSIZE, (right - left) * PTSIZE, perm);
	//kprintf("Step1 start 0x%08x, 0x%08x, 0x%08x\n", start, right, table);
	// Stop at the first non-empty entry before right
	while (start < right && !(table[start] & PTEX_P)) {
		start++;
	}
	//kprintf("Entry found.\n");
	if (start < right) {
		if (left_store != NULL) {
			//kprintf("set left store: 0x%08x\n", start);
			*left_store = start;
		}
		// get permission of the first entry
		int perm = (table[start++] & mask);
		//kprintf("entry: 0x%08x perm: 0x%08x\n", start - 1, perm);
		// Stop at the entry where permission is coherent
		// for PDT, it will be a big bank
		while (start < right && (table[start] & mask) == perm) {
			start++;
		}
		if (right_store != NULL) {
			//kprintf("set righ store: 0x%08x %03x\n", start, perm);
			*right_store = start;
		}
		return perm;
	}
	//kprintf("Entry out of bound.\n");
	return 0;
}

//print_pgdir - print the PDT&PT
void print_pgdir(int (*printf) (const char *fmt, ...))
{
	printf("-------------------- BEGIN --------------------\n");
	size_t left, right = 0, perm;
	while ((perm =
		get_pgtable_items(0, NPDEENTRY, right, boot_pgdir, &left,
				  &right, 0x3FF)) != 0) {
		printf("PDE(%03x) %08x-%08x %08x %s\n", right - left,
		       left * PTSIZE, right * PTSIZE, (right - left) * PTSIZE,
		       perm2str(perm));

		bool ref_valid = 0;
		size_t ref_l, ref_r, ref_perm;

		size_t curr_pt_id;
		for (curr_pt_id = left; curr_pt_id < right; curr_pt_id++) {
			pte_t *curr_pt =
			    (pte_t *) PDE_ADDR(boot_pgdir[curr_pt_id]);
			size_t range_perm;
			size_t l, r = 0;
			while ((range_perm =
				get_pgtable_items(0, NPTEENTRY, r, KADDR((uintptr_t)curr_pt), &l,
						  &r, PTEX_UW)) != 0) {
				size_t range_l =
				    curr_pt_id * PTSIZE + l * PGSIZE;
				size_t range_r =
				    curr_pt_id * PTSIZE + r * PGSIZE;
				if (!ref_valid) {
					ref_valid = 1;
					ref_l = range_l;
					ref_r = range_r;
					ref_perm = range_perm;
				} else if (range_l == ref_r
					   && range_perm == ref_perm) {
					ref_r = range_r;
				} else {
					int ref_dist = ref_r - ref_l;
					printf
					    ("  |-- PTE(%05x) %08x-%08x %08x %s\n",
					     ref_dist / PGSIZE, ref_l, ref_r,
					     ref_dist, perm2str(ref_perm));
					ref_l = range_l;
					ref_r = range_r;
					ref_perm = range_perm;
				}
			}
		}
		if (ref_valid) {
			int ref_dist = ref_r - ref_l;
			printf("  |-- PTE(%05x) %08x-%08x %08x %s\n",
			       ref_dist / PGSIZE, ref_l, ref_r, ref_dist,
			       perm2str(ref_perm));
		}
	}
	printf("--------------------- END ---------------------\n");
}

static uint32_t __current_ioremap_base = UCORE_IOREMAP_BASE;

void *__ucore_ioremap(unsigned long phys_addr, size_t size, unsigned int mtype)
{
	size = ROUNDUP(size, PGSIZE);
	if (__current_ioremap_base + size > UCORE_IOREMAP_END)
		return NULL;
	__boot_map_iomem(__current_ioremap_base, size, phys_addr);
	uint32_t oldaddr = __current_ioremap_base;
	__current_ioremap_base += size;

	tlb_invalidate_all();
	return (void *)oldaddr;
}
