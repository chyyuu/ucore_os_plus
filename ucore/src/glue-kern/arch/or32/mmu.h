#ifndef __KERN_MM_MMU_H__
#define __KERN_MM_MMU_H__

#define NR_SETS         64	// 64 set TLB for both data and instruction

/* page directory and page table constants */
#define NPDEENTRY       512	// page directory entries per page directory
#define NPTEENTRY       2048	// page table entries per page table

#define PGSIZE          8192	// bytes mapped by a page
#define PGSHIFT         13	// log2(PGSIZE)
#define PTSIZE          (PGSIZE * NPTEENTRY)	// bytes mapped by a page directory entry
#define PMSIZE			PTSIZE
#define PUSIZE			PTSIZE
#define PTSHIFT         24	// log2(PTSIZE)

#define SECTSIZE        512	// bytes of a sector
#define PAGE_NSECT      (PGSIZE / SECTSIZE)	// sectors per page

#define PTXSHIFT        13	// offset of PTX in a linear address
#define PDXSHIFT        24	// offset of PDX in a linear address
#define PMXSHIFT		PDXSHIFT
#define PUXSHIFT		PDXSHIFT
#define PGXSHIFT		PDXSHIFT

/* control bits in the pseudo page table*/

#define PTE_P           0x01
#define PTE_A           0x10
#define PTE_D           0x20
#define PTE_USER_R      0x40
#define PTE_USER_W      0x80
#define PTE_SPR_R       0x100
#define PTE_SPR_W       0x200
#define PTE_E           0x400
#define PTE_S           0x800

#define PTE_USER        (PTE_USER_R | PTE_USER_W | PTE_E | PTE_SPR_R | PTE_SPR_W | PTE_P)
#define PTE_PERM        (PTE_USER_R | PTE_USER_W | PTE_SPR_R | PTE_SPR_W | PTE_E)
#define PTE_SWAP        (PTE_A | PTE_D)

#ifndef __ASSEMBLER__

#include <types.h>

/* This variable is used by TLB miss exception handlers. */
extern uintptr_t current_pgdir_pa;
extern uintptr_t boot_pgdir_pa;

// A linear address 'la' has a three-part structure as follows:
//
// +--------8-------+-------11-------+---------13----------+
// | Page Directory |   Page Table   | Offset within Page  |
// |      Index     |     Index      |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(la) --/ \--- PTX(la) --/ \---- PGOFF(la) ----/
//  \----------- PPN(la) -----------/
//
// The PDX, PTX, PGOFF, and PPN macros decompose linear addresses as shown.
// To construct a linear address la from PDX(la), PTX(la), and PGOFF(la),
// use PGADDR(PDX(la), PTX(la), PGOFF(la)).

// page directory index
#define PDX(la) ((((uintptr_t)(la)) >> PDXSHIFT) & 0xFF)
#define PGX(la) PDX(la)
#define PUX(la) PDX(la)
#define PMX(la) PDX(la)

// page table index
#define PTX(la) ((((uintptr_t)(la)) >> PTXSHIFT) & 0x7FF)

// page number field of address
#define PPN(la) (((uintptr_t)(la)) >> PTXSHIFT)

// offset in page
#define PGOFF(la) (((uintptr_t)(la)) & 0x1FFF)

// construct linear address from indexes and offset
#define PGADDR(d, t, o) ((uintptr_t)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

// address in page table or page directory entry
#define PTE_ADDR(pte)   ((uintptr_t)(pte) & ~0x1FFF)
#define PDE_ADDR(pde)   PTE_ADDR(pde)
#define PMD_ADDR(pmd)   PTE_ADDR(pmd)
#define PUD_ADDR(pud)   PTE_ADDR(pud)
#define PGD_ADDR(pgd)   PTE_ADDR(pgd)

void tlb_invalidate_all();

#endif /* !__ASSEMBLER */

#endif /* !__KERN_MM_MMU_H__ */
