#include <types.h>
#include <arch.h>
#include <stdio.h>
#include <string.h>
#include <mmu.h>
#include <memlayout.h>
#include <pmm.h>
#include <buddy_pmm.h>
#include <sync.h>
#include <error.h>
#include <lapic.h>
#include <sysconf.h>
#include <memmap.h>
#include <spinlock.h>
#include <kern.h>

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
static struct taskstate ts[LAPIC_COUNT] = { {0} };

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
static struct segdesc gdt[8 + LAPIC_COUNT] = {
	SEG_NULL,
	[SEG_KTEXT] = SEG(STA_X | STA_R, DPL_KERNEL),
	[SEG_KDATA] = SEG(STA_W, DPL_KERNEL),
	[SEG_UTEXT] = SEG(STA_X | STA_R, DPL_USER),
	[SEG_UDATA] = SEG(STA_W, DPL_USER),
	[SEG_TLS1] = SEG(STA_W, DPL_USER),
	[SEG_TLS2] = SEG(STA_W, DPL_USER),
};

struct pseudodesc gdt_pd = {
	sizeof(gdt) - 1, (uintptr_t) gdt
};

static void check_alloc_page(void);
static void check_boot_pgdir(void);

/* *
 * lgdt - load the global descriptor table register and reset the
 * data/code segement registers for kernel.
 * */
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

/* *
 * load_esp0 - change the ESP0 in default task state segment,
 * so that we can use different kernel stack when we trap frame
 * user to kernel.
 * */
void load_rsp0(uintptr_t rsp0)
{
	ts[lapic_id()].ts_rsp0 = rsp0;
}

/* gdt_init - initialize the default GDT and TSS */
static void gdt_init(void)
{
	// initialize the TSS filed of the gdt
	int i;
	for (i = 0; i != LAPIC_COUNT; ++i) {
		gdt[SEG_TSS(i)] =
		    SEGTSS(STS_T32A, (uintptr_t) & ts[i], sizeof(ts[i]),
			   DPL_KERNEL);
	}

	// reload all segment registers
	lgdt(&gdt_pd);

	// load the TSS
	ltr(GD_TSS(sysconf.lcpu_boot));
}

//init_pmm_manager - initialize a pmm_manager instance
static void init_pmm_manager(void)
{
	pmm_manager = &buddy_pmm_manager;
	cprintf("memory management: %s\n", pmm_manager->name);
	pmm_manager->init();
	spinlock_init(&pmm_lock);
}

//init_memmap - call pmm->init_memmap to build Page struct for free memory  
static void init_memmap(struct Page *base, size_t n)
{
	pmm_manager->init_memmap(base, n);
}

/* pmm_init - initialize the physical memory management */
static void page_init(void)
{
	struct e820map *memmap = (struct e820map *)(0x8000 + PHYSBASE);
	uint64_t maxpa = 0;

	memmap_reset();

	cprintf("e820map:\n");
	int i;
	for (i = 0; i < memmap->nr_map; i++) {
		uint64_t begin = memmap->map[i].addr, end =
		    begin + memmap->map[i].size;

		if (end <= RESERVED_DRIVER_OS_SIZE)
			continue;
		if (begin < RESERVED_DRIVER_OS_SIZE)
			begin = RESERVED_DRIVER_OS_SIZE;

		cprintf("  memory: %016llx, [%016llx, %016llx], type = %d.\n",
			memmap->map[i].size, begin, end - 1,
			memmap->map[i].type);

		memmap_append(begin, end, memmap->map[i].type);

		if (memmap->map[i].type == E820_ARM) {
			if (maxpa < end && begin < SVSIZE) {
				maxpa = end;
			}
		}
	}
	if (maxpa > SVSIZE) {
		maxpa = SVSIZE;
	}

	memmap_process(1);
	extern char __end[];

	npage = (maxpa - RESERVED_DRIVER_OS_SIZE) / PGSIZE;
	pages = (struct Page *)ROUNDUP((uintptr_t) __end + SVBASE, PGSIZE);

	for (i = 0; i < npage; i++) {
		SetPageReserved(pages + i);
	}

	freemem =
	    ROUNDUP((uintptr_t) pages + sizeof(struct Page) * npage - SVBASE,
		    PGSIZE);

	for (i = 0; i < memmap->nr_map; i++) {
		uint64_t begin = memmap->map[i].addr, end =
		    begin + memmap->map[i].size;

		if (end <= RESERVED_DRIVER_OS_SIZE)
			continue;
		if (begin < RESERVED_DRIVER_OS_SIZE)
			begin = RESERVED_DRIVER_OS_SIZE;

		if (memmap->map[i].type == E820_ARM) {
			if (begin < freemem) {
				begin = freemem;
			}
			if (end > SVSIZE) {
				end = SVSIZE;
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

//boot_alloc_page - allocate one page using pmm->alloc_pages(1) 
// return value: the kernel virtual address of this allocated page
//note: this function is used to get the memory for PDT(Page Directory Table)&PT(Page Table)
static void *boot_alloc_page(void)
{
	struct Page *p = alloc_page();
	if (p == NULL) {
		panic("boot_alloc_page failed.\n");
	}
	return page2va(p);
}

static void
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
	boot_cr3 = PADDR_DIRECT(boot_pgdir);

	static_assert(SVBASE % PUSIZE == 0 && SVTOP % PUSIZE == 0);

	// recursively insert boot_pgdir in itself
	// to form a virtual page table at virtual address VPT
	boot_pgdir[PGX(VPT)] = PADDR_DIRECT(boot_pgdir) | PTE_P | PTE_W;

	// map all physical memory at KERNBASE
	boot_map_segment(boot_pgdir, PHYSBASE,
			 RESERVED_DRIVER_OS_SIZE + npage * PGSIZE, 0, PTE_W);

	extern char __sv_start[];
	uintptr_t sv_start = (uintptr_t) __sv_start;
	uintptr_t sv_space =
	    page2pa(alloc_pages((freemem - sv_start) / PGSIZE));
	boot_map_segment(boot_pgdir, SVBASE + sv_start, freemem - sv_start,
			 sv_space, PTE_W);
	memmove(VADDR_DIRECT(sv_space), (void *)(SVBASE + sv_start),
		freemem - sv_start);

	lcr3(boot_cr3);

	/* patched by xinhaoyuan: to fix the unmapped space when reading ACPI */
	/* configuration table under X86-64 */
	struct e820map *memmap = (struct e820map *)(0x8000 + PHYSBASE);
	int i;
	for (i = 0; i < memmap->nr_map; i++) {
		if (memmap->map[i].type != 1) {
			boot_map_segment(boot_pgdir,
					 memmap->map[i].addr + PHYSBASE,
					 memmap->map[i].size,
					 memmap->map[i].addr, PTE_W);
		}
	}
	/* and map the first 1MB for the ap booting */
	boot_map_segment(boot_pgdir, 0, 0x100000, 0, PTE_W);

	lcr3(boot_cr3);

	// set CR0
	uint64_t cr0 = rcr0();
	cr0 |=
	    CR0_PE | CR0_PG | CR0_AM | CR0_WP | CR0_NE | CR0_TS | CR0_EM |
	    CR0_MP;
	cr0 &= ~(CR0_TS | CR0_EM);
	lcr0(cr0);

	gdt_init();

	// check_boot_pgdir();

	print_pgdir(cprintf);
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
	cprintf("check_alloc_page() succeeded!\n");
}

/* disabled because of ... of what @_@ */
__attribute__ ((unused))
static void check_boot_pgdir(void)
{
	pte_t *ptep;
	int i;
	for (i = 0; i < npage; i += PGSIZE) {
		assert((ptep =
			get_pte(boot_pgdir, (uintptr_t) VADDR_DIRECT(i),
				0)) != NULL);
		assert(PTE_ADDR(*ptep) == i);
	}
	size_t nr_free_pages_saved = nr_free_pages();

	assert(PUD_ADDR(boot_pgdir[PGX(VPT)]) == PADDR_DIRECT(boot_pgdir));

	struct Page *p;
	p = alloc_page();
	assert(page_insert(boot_pgdir, p, 0x100, PTE_W) == 0);
	assert(page_ref(p) == 1);
	assert(page_insert(boot_pgdir, p, 0x100 + PGSIZE, PTE_W) == 0);
	assert(page_ref(p) == 2);

	const char *str = "ucore: Hello world!!";
	strcpy((void *)0x100, str);
	assert(strcmp((void *)0x100, (void *)(0x100 + PGSIZE)) == 0);

	*(char *)(page2va(p) + 0x100) = '\0';
	assert(strlen((const char *)0x100) == 0);

	free_page(pa2page(PTE_ADDR(*get_pte(boot_pgdir, 0x100, 0))));
	free_page(pa2page(PMD_ADDR(*get_pmd(boot_pgdir, 0x100, 0))));
	free_page(pa2page(PUD_ADDR(*get_pud(boot_pgdir, 0x100, 0))));
	free_page(pa2page(PGD_ADDR(*get_pgd(boot_pgdir, 0x100, 0))));
	boot_pgdir[0] = 0;

	assert(nr_free_pages() == nr_free_pages_saved);

	cprintf("check_boot_pgdir() succeeded!\n");
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
	printf("-------------------- BEGIN --------------------\n");
	print_pgdir_sub(sizeof(s1) / sizeof(s1[0]), 0, NPGENTRY, s1, s2, s3,
			printf);
	printf("--------------------- END ---------------------\n");
}

EXPORT_SYMBOL(print_pgdir);

struct segdesc *get_sv_gdt()
{
	return gdt;
}

EXPORT_SYMBOL(get_sv_gdt);
