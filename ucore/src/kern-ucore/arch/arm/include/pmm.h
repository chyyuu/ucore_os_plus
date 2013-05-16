#ifndef __KERN_MM_PMM_H__
#define __KERN_MM_PMM_H__

#include <types.h>
#include <mmu.h>
#include <memlayout.h>
#include <proc.h>
#include <atomic.h>
#include <assert.h>

// pmm_manager is a physical memory management class. A special pmm manager - XXX_pmm_manager
// only needs to implement the methods in pmm_manager class, then XXX_pmm_manager can be used
// by ucore to manage the total physical memory space.
struct pmm_manager {
	const char *name;
	void (*init) (void);
	void (*init_memmap) (struct Page * base, size_t n);	// setup description&management data structcure according to
	struct Page *(*alloc_pages) (size_t n);
	void (*free_pages) (struct Page * base, size_t n);
	 size_t(*nr_free_pages) (void);
	void (*check) (void);
};

extern const struct pmm_manager *pmm_manager;
extern pde_t *boot_pgdir;

void pmm_init(void);

struct Page *alloc_pages(size_t n);
void free_pages(struct Page *base, size_t n);
size_t nr_free_pages(void);

#define alloc_page() alloc_pages(1)
#define free_page(page) free_pages(page, 1)

pgd_t *get_pgd(pgd_t * pgdir, uintptr_t la, bool create);
pud_t *get_pud(pgd_t * pgdir, uintptr_t la, bool create);
pmd_t *get_pmd(pgd_t * pgdir, uintptr_t la, bool create);
pte_t *get_pte(pde_t * pgdir, uintptr_t la, bool create);
struct Page *get_page(pde_t * pgdir, uintptr_t la, pte_t ** ptep_store);
void page_remove(pde_t * pgdir, uintptr_t la);

#ifdef UCONFIG_BIONIC_LIBC
void page_insert_pte(pde_t * pgdir, struct Page *page, pte_t * ptep,
		     uintptr_t la, uint32_t perm);
#endif //UCONFIG_BIONIC_LIBC
int page_insert(pde_t * pgdir, struct Page *page, uintptr_t la, uint32_t perm);

void tlb_invalidate(pde_t * pgdir, uintptr_t la);
void tlb_clean_flush(pde_t * pgdir, uintptr_t la);

void load_rsp0(uintptr_t esp0);
void set_pgdir(struct proc_struct *proc, pde_t * pgdir);
void load_pgdir(struct proc_struct *proc);
void map_pgdir(pde_t * pgdir);

struct Page *pgdir_alloc_page(pde_t * pgdir, uintptr_t la, uint32_t perm);
//void unmap_range(pde_t *pgdir, uintptr_t start, uintptr_t end);
//void exit_range(pde_t *pgdir, uintptr_t start, uintptr_t end);
//int copy_range(pde_t *to, pde_t *from, uintptr_t start, uintptr_t end, bool share);

//void print_pgdir(void);
void print_pgdir(int (*printf) (const char *fmt, ...));

/* *
 * PADDR - takes a kernel virtual address (an address that points above KERNBASE),
 * where the machine's maximum 256MB of physical memory is mapped and returns the
 * corresponding physical address.  It panics if you pass it a non-kernel virtual address.
 * Current implementation puts kernel in fixed address
 * */
//TODO KERNBASE?
#define PADDR(kva) ({                                                   \
            uintptr_t __m_kva = (uintptr_t)(kva);                       \
            if (__m_kva < KERNBASE) {                                   \
                panic("PADDR called with invalid kva 0x%08lx", __m_kva);\
            }                                                           \
            __m_kva - KERNBASE;                                         \
        })

/* *
 * KADDR - takes a physical address and returns the corresponding kernel virtual
 * address. It panics if you pass an invalid physical address.
 * Current implementation puts kernel in fixed address
 * */
#define KADDR(pa) ({                                                    \
            uintptr_t __m_pa = (pa);                                    \
            size_t __m_ppn = PPN(__m_pa);                               \
            if (__m_ppn >= npage) {                                     \
                panic("KADDR called with invalid pa 0x%08lx", __m_pa);  \
            }                                                           \
            (void *) (__m_pa + KERNBASE);                                          \
        })

#define NEXT_PAGE(pg) (pg + 1)

extern struct Page *pages;
extern size_t npage;

// physical address of page to page number
static inline ppn_t page2ppn(struct Page *page)
{
	return page - pages;
}

// physical address of page to pointing address of page
static inline uintptr_t page2pa(struct Page *page)
{
	return page2ppn(page) << PGSHIFT;
}

static inline struct Page *pa2page(uintptr_t pa)
{
	if (PPN(pa) >= npage) {
		panic("pa2page called with invalid 0x%08lx", pa);
	}
	return &pages[PPN(pa)];
}

static inline void *page2kva(struct Page *page)
{
	return KADDR(page2pa(page));
}

static inline struct Page *kva2page(void *kva)
{
	return pa2page(PADDR(kva));
}

static inline struct Page *pte2page(pte_t pte)
{
	if (!(pte & PTEX_P)) {	// Can't use status table
		panic("pte2page called with invalid pte 0x%08lx", pte);
	}
	return pa2page(PTE_ADDR(pte));
}

static inline struct Page *pde2page(pde_t pde)
{
	return pa2page(PDE_ADDR(pde));
}

static inline struct Page *pmd2page(pmd_t pmd)
{
	return pa2page(PMD_ADDR(pmd));
}

static inline struct Page *pud2page(pud_t pud)
{
	return pa2page(PUD_ADDR(pud));
}

static inline struct Page *pgd2page(pgd_t pgd)
{
	return pa2page(PGD_ADDR(pgd));
}

static inline int page_ref(struct Page *page)
{
	return atomic_read(&(page->ref));
}

static inline void set_page_ref(struct Page *page, int val)
{
	atomic_set(&(page->ref), val);
}

static inline int page_ref_inc(struct Page *page)
{
	return atomic_add_return(&(page->ref), 1);
}

static inline int page_ref_dec(struct Page *page)
{
	return atomic_sub_return(&(page->ref), 1);
}

struct user_tls_desc {
	uint32_t entry_number;
	uintptr_t base_addr;
	size_t limit_t;
	unsigned int seg_32bit:1;
	unsigned int contents:2;
	unsigned int read_exec_only:1;
	unsigned int limit_in_pages:1;
	unsigned int seg_not_present:1;
	unsigned int useable:1;
	unsigned int empty:25;
};

uint32_t do_set_tls(struct user_tls_desc *tlsp);

static inline pgd_t *init_pgdir_get(void)
{
	return (pgd_t *)KADDR( boot_pgdir_pa);
}

void pmm_init_ap(void);

extern char bootstack[], bootstacktop[];

void *__ucore_ioremap(unsigned long phys_addr, size_t size, unsigned int mtype);

#endif /* !__KERN_MM_PMM_H__ */
