#include <arch.h>
#include <pmm.h>
#include <mmu.h>
#include <vmm.h>
#include <memlayout.h>
#include <error.h>
#include <buddy_pmm.h>
#include <stdio.h>
#include <sync.h>
#include <types.h>
#include <stub.h>
#include <string.h>
#include <host_syscall.h>
#include <kio.h>
#include <glue_mp.h>
#include <proc.h>
#include <slab.h>

#define current pls_read(current)

extern struct mm_struct *check_mm_struct;

// size of memory above 0x100000
size_t mem_size = 0;
char *bootstack = (char *)KSTACK, *bootstacktop = (char *)KSTACKTOP;

/**************************************************
 * Page table operations
 **************************************************/

/**
 * Find a proc_struct whose mm->pgdir is @pgdir.
 * Note that if multiple proc_structs have the same pgdir, then they must be threads of the same group,
 *     and thus are in the same container process.
 * @param pgdir the 'hint' page table for finding the target proc_struct
 * @return the pointer to the proc_struct found,
 *             or NULL if the map/uemap should be done in the main process
 */
static struct proc_struct *find_proc_by_pgdir(pde_t * pgdir)
{
	/* Some special occasions:
	 *     1. current == NULL means we're work in test functions.
	 *            Then the map/unmap belongs to the main process (we only have it at that time).
	 *     2. current->mm == NULL means current is a kernel thread.
	 *            The only kernel thread which needs to modify user address space is kswapd
	 *                 for it should invalidate the page which has been swapped out.
	 *            The others should be only occured when creating a user process by 'do_execve'
	 *                 and the work belongs to the host process.
	 *     3. if current->mm->pgdir = @pgdir, which is commonly the case,
	 *            'current' is obviously what we are looking for.
	 *            This is just for speeding up.
	 */
	if (current != kswapd) {
		if (current == NULL || current->mm == NULL)
			return NULL;
		if (pgdir == current->mm->pgdir)
			return current;
	}

	/* Search for a proc_sturct by brute force,
	 *     iterating the proc list and ask everyone.
	 */
	list_entry_t *le = &proc_list;
	if (list_next(le) == 0)	/* not initialized. this happens when kswapd is initializing */
		return NULL;
	struct proc_struct *proc;
	while ((le = list_next(le)) != &proc_list) {
		proc = le2proc(le, list_link);
		if (proc->mm != NULL && proc->mm->pgdir == pgdir) {
			return proc;
		}
	}
	return NULL;
}

/**
 * Remap the specified address to a new page with new permission.
 * @param pgdir   page directory
 * @param la      linear address
 */
void tlb_update(pde_t * pgdir, uintptr_t la)
{
	la = ROUNDDOWN(la, PGSIZE);
	pte_t *pte = get_pte(pgdir, la, 0);
	if (pte == 0 || (*pte & PTE_P) == 0)
		panic("invalid tlb flushing\n");
	uint32_t pa = PDE_ADDR(*pte);

	/* A tricky method to make the page table right under most circumstances.
	 *     Please consult the internal documentation for details.
	 */
	int r = 1, w = 1, x = 1;
	if (Get_PTE_A(pte) == 0)
		r = x = w = 0;
	else if (Get_PTE_W(pte) == 0 || Get_PTE_D(pte) == 0)
		w = 0;

	/* Make sure that the page is invalid before mapping
	 *     It is better to use 'mprotect' here actually.
	 */
	tlb_invalidate(pgdir, la);

	struct proc_struct *proc = find_proc_by_pgdir(pgdir);
	if (current != NULL && proc != NULL) {
		/* Map the page to the container process found using the stub code */
		if (host_mmap(proc,
			      (void *)la, PGSIZE,
			      (r ? PROT_READ : 0) | (w ? PROT_WRITE : 0) | (x ?
									    PROT_EXEC
									    :
									    0),
			      MAP_SHARED | MAP_FIXED, ginfo->mem_fd,
			      pa) == MAP_FAILED)
			panic("map in child failed.\n");
	} else {
		/* Map the page to the host process */
		struct mmap_arg_struct args = {
			.addr = la,
			.len = PGSIZE,
			.prot =
			    (r ? PROT_READ : 0) | (w ? PROT_WRITE : 0) | (x ?
									  PROT_EXEC
									  : 0),
			.flags = MAP_SHARED | MAP_FIXED,
			.fd = ginfo->mem_fd,
			.offset = pa,
		};
		syscall1(__NR_mmap, (long)&args);
	}
}

/**
 * unmap the page specified by @la in the container process corresponding to @pgdir
 * @param pgdir page directory
 * @param la the logical address of the page to be flushed
 */
void tlb_invalidate(pde_t * pgdir, uintptr_t la)
{
	struct proc_struct *proc = find_proc_by_pgdir(pgdir);
	if (current != NULL && proc != NULL) {
		if (host_munmap(proc, (void *)la, PGSIZE) < 0)
			panic("unmap in child failed\n");
	} else {
		syscall2(__NR_munmap, la, PGSIZE);
	}
}

/**
 * invalidate [USERBASE, USERTOP).
 * used by tests or do_execve if a 'clean' space is needed (though not neccesary).
 */
void tlb_invalidate_user(void)
{
	syscall2(__NR_munmap, USERBASE, USERTOP - USERBASE);
}

/**************************************************
 * Memory tests
 **************************************************/

/**
 * Check whether the virtual physical memory created works as designed.
 */
static void check_vpm(void)
{
	int *p;
	uint32_t freemem = e820map.nr_map - 1;
	uint32_t freemem_start = (uint32_t) e820map.map[freemem].addr;
	uint32_t freemem_size = (uint32_t) e820map.map[freemem].size;

	/* write at the beginning */
	p = (int *)freemem_start;
	*p = 55;
	assert(*p == 55);

	/* write at the middle */
	p = (int *)(freemem_start + freemem_size / 2);
	*p = 1111;
	assert(*p == 1111);

	/* write at the end */
	p = (int *)((uint32_t) (freemem_size + freemem_start - sizeof(int)));
	*p = 101;
	assert(*p = 101);

	kprintf("check_vpm() succeeded.\n");
}

/**
 * Check whether page directory for boot lives well.
 *     NOTE: we don't have mm_struct at present.
 *           as write to a clean page also raises SIGSEGV, we're not able to deal with it now.
 *           so just mark all page inserted to be accessed and dirty.
 */
void check_boot_pgdir(void)
{
	pte_t *ptep;
	int i;
	for (i = 0; i < npage; i += PGSIZE) {
		assert((ptep =
			get_pte(boot_pgdir, (uintptr_t) KADDR(i), 0)) != NULL);
		assert(PTE_ADDR(*ptep) == i);
	}

	//assert(PDE_ADDR(boot_pgdir[PDX(VPT)]) == PADDR(boot_pgdir));

	assert(boot_pgdir[PDX(TEST_PAGE)] == 0);

	struct Page *p;
	p = alloc_page();
	assert(page_insert(boot_pgdir, p, TEST_PAGE, PTE_W | PTE_D | PTE_A) ==
	       0);
	assert(page_ref(p) == 1);
	assert(page_insert
	       (boot_pgdir, p, TEST_PAGE + PGSIZE, PTE_W | PTE_D | PTE_A) == 0);
	assert(page_ref(p) == 2);

	const char *str = "ucore: Hello world!!";
	strcpy((void *)TEST_PAGE, str);
	assert(strcmp((void *)TEST_PAGE, (void *)(TEST_PAGE + PGSIZE)) == 0);

	*(char *)(page2kva(p)) = '\0';
	assert(strlen((const char *)TEST_PAGE) == 0);

	/*
	 * in um architecture clear page table doesn't mean
	 *     the linear address is invalid
	 * so remove them by hand
	 */
	tlb_invalidate(boot_pgdir, TEST_PAGE);
	tlb_invalidate(boot_pgdir, TEST_PAGE + PGSIZE);

	free_page(p);
	free_page(pa2page(PDE_ADDR(boot_pgdir[PDX(TEST_PAGE)])));
	boot_pgdir[PDX(TEST_PAGE)] = 0;

	kprintf("check_boot_pgdir() succeeded.\n");
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

/**************************************************
 * Empty functions
 *     They are used by other architectures.
 **************************************************/
void load_rsp0(uintptr_t rsp0)
{
}

void set_pgdir(struct proc_struct *proc, pde_t * pgdir)
{
}

void load_pgdir(struct proc_struct *proc)
{
}

void map_pgdir(pde_t * pgdir)
{
}

/**
 * Examine address space, create virtual physical memory and map it.
 */
static void page_init(void)
{
	int i;
	int freemem_size = 0;

	/* Construct page descriptor table.
	 * mem => memory not reserved or occupied by kernel code
	 * freemem => memory available after page descriptor table is built
	 */

	/* all pages from 0x100000 to the top should have an entry in page descriptor table */
	for (i = 0; i < e820map.nr_map; i++) {
		mem_size += (uint32_t) (e820map.map[i].size);
		if (e820map.map[i].type == E820_ARM)
			freemem_size += e820map.map[i].size;
	}

	pages =
	    (struct Page *)(uint32_t) (e820map.map[e820map.nr_map - 1].addr);
	npage = (mem_size) / PGSIZE;
	for (i = 0; i < npage; i++) {
		SetPageReserved(pages + i);
	}

	uintptr_t freemem =
	    PADDR(ROUNDUP
		  ((uintptr_t) pages + sizeof(struct Page) * npage, PGSIZE));
	uint32_t freemem_npage =
	    freemem_size / PGSIZE - npage * sizeof(struct Page) / PGSIZE;
	init_memmap(pa2page(freemem), freemem_npage);
}

/**
 * Make the permission printable.
 * @param perm the permission bits of the page
 * @return a printable string for the permission
 */
static const char *perm2str(int perm)
{
	static char str[4];
	str[0] = (perm & PTE_U) ? 'u' : '-';
	str[1] = 'r';
	str[2] = (perm & PTE_W) ? 'w' : '-';
	str[3] = '\0';
	return str;
}

/**
 * Lookup the page directory and print valid entries.
 */
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

/**
 * Lookup the page table and print valid entries.
 *     Note: I can not make any clever method to work correctly. So this is the most ugly way.
 */
static int
get_pgtable_items_2(size_t left, size_t right, size_t start, uintptr_t * pgdir,
		    size_t * left_store, size_t * right_store)
{
	pte_t *pte;

	if (start >= right) {
		return 0;
	}
	while (start < right &&
	       ((pte = get_pte(pgdir, start * PGSIZE, 0)) == NULL
		|| !(*pte & PTE_P)))
		start++;
	if (start < right) {
		if (left_store != NULL)
			*left_store = start;
		int perm = *get_pte(pgdir, (start++) * PGSIZE, 0) & PTE_USER;
		while (start < right &&
		       (pte = get_pte(pgdir, start * PGSIZE, 0)) != NULL
		       && (*pte & PTE_USER) == perm) {
			start++;
		}
		if (right_store != NULL) {
			*right_store = start;
		}
		return perm;
	}
	return 0;
}

/**
 * Print the page table.
 *     If no current->mm->pgdir, print boot_pgdir, or print the pgdir of the current user process instead.
 */
void print_pgdir(int (*printf) (const char *fmt, ...))
{
	printf("-------------------- BEGIN --------------------\n");
	size_t left, right = 0, perm;
	pde_t *pgdir = boot_pgdir;
	if (current != NULL && current->mm != NULL
	    && current->mm->pgdir != NULL) {
		pgdir = current->mm->pgdir;
	}

	while ((perm =
		get_pgtable_items(0, NPDEENTRY, right, pgdir, &left,
				  &right)) != 0) {
		printf("PDE(%03x) %08x-%08x %08x %s\n", right - left,
		       left * PTSIZE, right * PTSIZE, (right - left) * PTSIZE,
		       perm2str(perm));
		size_t l, r = left * NPTEENTRY;
		while ((perm =
			get_pgtable_items_2(left * NPTEENTRY, right * NPTEENTRY,
					    r, pgdir, &l, &r)) != 0) {
			printf("  |-- PTE(%05x) %08x-%08x %08x %s\n", r - l,
			       l * PGSIZE, r * PGSIZE, (r - l) * PGSIZE,
			       perm2str(perm));
		}
	}
	printf("--------------------- END ---------------------\n");
}

/**
 * Initialize page management mechanism.
 *     Parts of no use are deleted, while no extra parts except a check is added.
 *     arch/x86/mm/pmm.c should be a good reference.
 */
void pmm_init(void)
{
	check_vpm();

	init_pmm_manager();

	page_init();

	check_alloc_page();

	boot_pgdir = boot_alloc_page();
	memset(boot_pgdir, 0, PGSIZE);
	check_pgdir();

	/* register kernel code and data pages in the table so that it won't raise bad segv. */
	boot_map_segment(boot_pgdir, KERNBASE, mem_size, 0, PTE_W);

	check_boot_pgdir();
	print_pgdir(kprintf);

	slab_init();
}
