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
#include <sysconf.h>
#include <ramdisk.h>

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
 * privilege level change happens. But only the fields SS0 and RSP0 are useful
 * in our os kernel.
 *
 * The field SS0 contains the stack segment selector for CPL = 0, and the RSP0
 * contains the new RSP value for CPL = 0. When an interrupt happens in protected
 * mode, the x86-64 CPU will look in the TSS for SS0 and RSP0 and load their value
 * into SS and RSP respectively.
 * */
//static struct taskstate ts = {0};
uintptr_t boot_cr3;

static uintptr_t freemem;

pte_t *const vpt = (pte_t *) VPT;
pmd_t *const vmd = (pmd_t *) PGADDR(PGX(VPT), PGX(VPT), 0, 0, 0);
pud_t *const vud = (pud_t *) PGADDR(PGX(VPT), PGX(VPT), PGX(VPT), 0, 0);
pgd_t *const vgd = (pgd_t *) PGADDR(PGX(VPT), PGX(VPT), PGX(VPT), PGX(VPT), 0);

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
static struct segdesc __gdt[SEG_COUNT] = {
	SEG_NULL,
	[SEG_KTEXT] = SEG(STA_X | STA_R, DPL_KERNEL),
	[SEG_KDATA] = SEG(STA_W, DPL_KERNEL),
	[SEG_UTEXT] = SEG(STA_X | STA_R, DPL_USER),
	[SEG_UDATA] = SEG(STA_W, DPL_USER),
	/*
	[SEG_TLS1] = SEG(STA_W, DPL_USER),
	[SEG_TLS2] = SEG(STA_W, DPL_USER),
	*/
};

#if 0
static struct pseudodesc __gdt_pd = {
	sizeof(gdt) - 1, (uintptr_t) gdt
};
#endif

static DEFINE_PERCPU_NOINIT(size_t, used_pages);
DEFINE_PERCPU_NOINIT(list_entry_t, page_struct_free_list);

// virtual address of physical page array
struct Page *pages;
// amount of physical memory (in pages)
size_t npage = 0;
unsigned long max_pfn;

// virtual address of boot-time page directory
pgd_t *boot_pgdir = NULL;

// physical memory management
const struct pmm_manager *pmm_manager;

static void check_alloc_page(void);

/* *
 * lgdt - load the global descriptor table register and reset the
 * data/code segement registers for kernel.
 * */
static inline void lgdt(struct pseudodesc *pd)
{
	asm volatile ("lgdt (%0)"::"r" (pd));
	//XXX needed? may confuse the compiler
#if 0
	asm volatile ("movw %%ax, %%es"::"a" (KERNEL_DS));
	asm volatile ("movw %%ax, %%ds"::"a" (KERNEL_DS));
	asm volatile ("movw %%ax, %%gs"::"a" (KERNEL_DS));

	// reload cs & ss
	asm volatile ("movq %%rsp, %%rax;"	// move %rsp to %rax
		      "pushq %1;"	// push %ss
		      "pushq %%rax;"	// push %rsp
		      "pushfq;"	// push %rflags
		      "pushq %0;"	// push %cs
		      "call 1f;"	// push %rip
		      "jmp 2f;"
		      "1: iretq; 2:"::"i" (KERNEL_CS), "i"(KERNEL_DS));
#endif
}

/* *
 * load_rsp0 - change the ESP0 in default task state segment,
 * so that we can use different kernel stack when we trap frame
 * user to kernel.
 * */
void load_rsp0(uintptr_t rsp0)
{
	//XXX
	mycpu()->arch_data.ts.ts_rsp0 = rsp0;
}

/**
 * set_pgdir - save the physical address of the current pgdir
 */
void set_pgdir(struct proc_struct *proc, pgd_t * pgdir)
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
		lcr3(PADDR(init_pgdir_get()));
}

/**
 * map_pgdir - map the current pgdir @pgdir to its own address space
 */
void map_pgdir(pgd_t * pgdir)
{
	ptep_map(&(pgdir[PGX(VPT)]), PADDR(pgdir));
	ptep_set_s_write(&(pgdir[PGX(VPT)]));
}

size_t nr_used_pages(void)
{
	return get_cpu_var(used_pages);
}

/* gdt_init - initialize the default GDT and TSS */
/* must be called from corresponding cpu */
void gdt_init(struct cpu *c)
{
	// initialize the TSS filed of the gdt
	// XXX
	//gdt[SEG_TSS] =
	//	SEGTSS(STS_T32A, (uintptr_t) & ts, sizeof(ts), DPL_KERNEL);

	memcpy(&c->arch_data.gdt, &__gdt, sizeof(__gdt));
	c->arch_data.gdt[SEG_TSS] = 
		SEGTSS(STS_T32A, (uintptr_t) &c->arch_data.ts,
		sizeof(struct taskstate), DPL_KERNEL);

	struct pseudodesc gdt_pd = {
		sizeof(__gdt) - 1, (uintptr_t) c->arch_data.gdt
	};

	// reload all segment registers
	lgdt(&gdt_pd);

	// load the TSS
	ltr((uint16_t)GD_TSS);
}

//init_pmm_manager - initialize a pmm_manager instance
static void init_pmm_manager(void)
{
	pmm_manager = &buddy_pmm_manager;
	kprintf("memory management: %s\n", pmm_manager->name);
	pmm_manager->init();
}

//init_memmap - call pmm->init_memmap to build Page struct for free memory  
static inline void init_memmap(struct numa_mem_zone *z)
{
	pmm_manager->init_memmap(z);
}

char* e820map_type[5]={"Usable","Reserved","ACPI reclaimable memory","ACPI NVS memory", "Area containing bad memory"};

#define MAX(x,y) ((x)<(y)?(y):(x))
#define MIN(x,y) ((x)>(y)?(y):(x))
struct numa_mem_zone numa_mem_zones[MAX_NUMA_MEM_ZONES];
static int numa_mem_zones_cnt;

/* pmm_init - initialize the physical memory management */
extern struct e820map *e820map_addr;
static void page_init(void)
{
	struct e820map *memmap = e820map_addr;
	uint64_t maxpa = 0, totalmemsize=0;

	kprintf("e820map: size, begin, end, type\n");
	kprintf("----------------------------------------\n");
	int i, j, k;
	for (i = 0; i < memmap->nr_map; i++) {
		uint64_t begin = memmap->map[i].addr, end = begin + memmap->map[i].size;
		kprintf("  memory: %016llx, [%016llx, %016llx], %s.\n",
			memmap->map[i].size, begin, end - 1,
			e820map_type[memmap->map[i].type - 1]);
		if (memmap->map[i].type == E820_ARM) {
			totalmemsize+=memmap->map[i].size;
			if (maxpa < end && begin < KMEMSIZE) {
				maxpa = end;
			}
		}
	}
	kprintf("--------Total Usable Phy Mem Size %lld MB-----------\n", totalmemsize/1024/1024);
	if (maxpa > KMEMSIZE) {
		kprintf("warning: memory size > 0x%llx\n", KMEMSIZE);
		maxpa = KMEMSIZE;
	}

	extern char end[];
	char *reserve_end = end;
	if(initrd_end){
		assert(initrd_end > end);
		reserve_end = initrd_end;
	}

	npage = maxpa / PGSIZE;
	max_pfn = npage;
	pages = (struct Page *)ROUNDUP((uintptr_t)reserve_end, PGSIZE);

	for (i = 0; i < npage; i++) {
		SetPageReserved(pages + i);
	}

	freemem = PADDR(ROUNDUP((uintptr_t) pages + sizeof(struct Page) * npage, PGSIZE));

	for (i = 0; i < memmap->nr_map; i++) {
		/* skip the first 1MB */
		uint64_t begin = memmap->map[i].addr, end = begin + memmap->map[i].size;
		if (end <= 0x100000)
			continue;
		if (memmap->map[i].type != E820_ARM)
			continue;
		begin = MAX(begin, freemem);
		end = MIN(end, KMEMSIZE);
		if (begin >= end)
			continue;
		begin = ROUNDUP(begin, PGSIZE);
		end = ROUNDDOWN(end, PGSIZE);
		if (begin >= end)
			continue;
		for(j=0; j<sysconf.lnuma_count; j++){
			struct numa_node *node = &numa_nodes[j];
			for(k=0;k<numa_nodes[j].nr_mems;k++){
				uint64_t l = MAX(begin, (uint64_t)node->mems[k].base),
					 r = MIN(end, (uint64_t)node->mems[k].base + node->mems[k].length);
				l = ROUNDUP(l, PGSIZE);
				r = ROUNDDOWN(r, PGSIZE);
				if(l>=r)
					continue;
				struct numa_mem_zone *z = &numa_mem_zones[numa_mem_zones_cnt++];
				z->id = numa_mem_zones_cnt - 1;
				z->page = pa2page(l);
				z->n = (r - l) / PGSIZE;
				z->node = node;
				//init_memmap(pa2page(begin),
				//		(end - begin) / PGSIZE);
			}
		}
	}
	for(i=0;i<numa_mem_zones_cnt;i++){
		kprintf("numa_mem_zone %d: 0x%016llx - 0x%016llx, %lldMB\n",
				i, page2pa(numa_mem_zones[i].page),
				numa_mem_zones[i].n * PGSIZE - 1, numa_mem_zones[i].n*PGSIZE/1024/1024);
		init_memmap(&numa_mem_zones[i]);
	}
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


struct Page *alloc_pages_cpu(struct cpu *cpu, size_t n)
{
#ifdef UCONFIG_SWAP
#error alloc_pages_numa: swap not supported
#endif
	struct Page *page;
	bool intr_flag;
	assert(cpu->node!=NULL);
	local_intr_save(intr_flag);
	{
		page = pmm_manager->alloc_pages_numa(cpu->node, n);
	}
	local_intr_restore(intr_flag);
	per_cpu(used_pages, cpu->id) += n;
	return page;
}

struct Page *alloc_pages_numa(struct numa_node* node, size_t n)
{
#ifdef UCONFIG_SWAP
#error alloc_pages_numa: swap not supported
#endif
	struct Page *page;
	bool intr_flag;
	assert(node!=NULL);
	local_intr_save(intr_flag);
	{
		page = pmm_manager->alloc_pages_numa(node, n);
	}
	local_intr_restore(intr_flag);
	return page;
}


//boot_alloc_page - allocate one page using pmm->alloc_pages(1) 
// return value: the kernel virtual address of this allocated page
//note: this function is used to get the memory for PDT(Page Directory Table)&PT(Page Table)
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

	void
boot_map_segment(pgd_t * pgdir, uintptr_t la, size_t size, uintptr_t pa,
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

//pmm_init - setup a pmm to manage physical memory, build PDT&PT to setup paging mechanism 
//         - check the correctness of pmm & paging mechanism, print PDT&PT
void pmm_init_numa(void)
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
	boot_cr3 = PADDR_DIRECT(boot_pgdir);

	// recursively insert boot_pgdir in itself
	// to form a virtual page table at virtual address VPT
	boot_pgdir[PGX(VPT)] = PADDR_DIRECT(boot_pgdir) | PTE_P | PTE_W;

	// map all physical memory at KERNBASE
	boot_map_segment(boot_pgdir, PBASE, npage * PGSIZE, 0, PTE_W);

	lcr3(boot_cr3);

	/* patched by xinhaoyuan: to fix the unmapped space when reading ACPI */
	/* configuration table under X86-64 */
	struct e820map *memmap = e820map_addr;
	int i;
	for (i = 0; i < memmap->nr_map; i++) {
		if (memmap->map[i].type != 1) {
			boot_map_segment(boot_pgdir,
					 memmap->map[i].addr + PBASE,
					 memmap->map[i].size,
					 memmap->map[i].addr, PTE_W);
		}
	}
	/* and map the first 1MB for the ap booting */
	/* boot_map_segment(boot_pgdir, 0, 0x100000, 0, PTE_W); */

	//TODO put here?
	pmm_init_ap();

	print_pgdir(kprintf);
	slab_init();
}

void pmm_init_ap(void)
{

	mp_lcr3(boot_cr3);

	// set CR0
	uint64_t cr0 = rcr0();
	cr0 |=
	    CR0_PE | CR0_PG | CR0_AM | CR0_WP | CR0_NE | CR0_TS | CR0_EM |
	    CR0_MP;
	cr0 &= ~(CR0_TS | CR0_EM);
	lcr0(cr0);

	//gdt_init();

	list_entry_t *page_struct_free_list =
	    get_cpu_ptr(page_struct_free_list);
	list_init(page_struct_free_list);
	get_cpu_var(used_pages) = 0;
}

// invalidate a TLB entry, but only if the page tables being
// edited are the ones currently in use by the processor.
void tlb_update(pgd_t * pgdir, uintptr_t la)
{
	tlb_invalidate(pgdir, la);
}

// invalidate a TLB entry, but only if the page tables being
// edited are the ones currently in use by the processor.
void tlb_invalidate(pgd_t * pgdir, uintptr_t la)
{
	if (rcr3() == PADDR_DIRECT(pgdir)) {
		invlpg((void *)la);
	}
}

static void check_alloc_page(void)
{
	pmm_manager->check();
	kprintf("check_alloc_page() succeeded!\n");
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

static void
print_pgdir_sub(int deep, size_t left, size_t right,
		char *s1[], size_t s2[], uintptr_t * s3[],
		int (*printf) (const char *fmt, ...))
{
	if (deep > 0) {
		uint32_t perm;
		size_t l, r = left;
		while ((perm =
			get_pgtable_items(left, right, r, s3[0], &l,
					  &r)) != 0) {
			printf(s1[0], r - l);
			size_t lb = l * s2[0], rb = r * s2[0];
			if ((lb >> 32) & 0x8000) {
				lb |= (0xFFFFLLU << 48);
				rb |= (0xFFFFLLU << 48);
			}
			printf(" %016llx-%016llx %016llx %s\n", lb, rb, rb - lb,
			       perm2str(perm));
			print_pgdir_sub(deep - 1, l * NPGENTRY, r * NPGENTRY,
					s1 + 1, s2 + 1, s3 + 1, printf);
		}
	}
}

//print_pgdir - print the PDT&PT
void print_pgdir(int (*printf) (const char *fmt, ...))
{
	char *s1[] = {
		"PGD          (%09x)",
		" |-PUD       (%09x)",
		" |--|-PMD    (%09x)",
		" |--|--|-PTE (%09x)",
	};
	size_t s2[] = { PUSIZE, PMSIZE, PTSIZE, PGSIZE };
	uintptr_t *s3[] = { vgd, vud, vmd, vpt };
	printf("PageTable: (num of items), mem-range, mem size, permission \n");
	printf("----------------------------------------\n");
	print_pgdir_sub(sizeof(s1) / sizeof(s1[0]), 0, NPGENTRY, s1, s2, s3,
			printf);
	printf("------------------------------------------\n");
}
