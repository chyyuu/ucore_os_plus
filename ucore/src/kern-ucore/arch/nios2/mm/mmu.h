#ifndef __KERN_MM_MMU_H__
#define __KERN_MM_MMU_H__

// A linear address 'la' has a three-part structure as follows:
//
// +--------10------+-------10-------+---------12----------+
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
#define PDX(la) ((((uintptr_t)(la)) >> PDXSHIFT) & 0x3FF)

// page table index
#define PTX(la) ((((uintptr_t)(la)) >> PTXSHIFT) & 0x3FF)

// page number field of address
#define PPN(la) (((uintptr_t)(la)) >> PTXSHIFT)

// offset in page
#define PGOFF(la) (((uintptr_t)(la)) & 0xFFF)

// construct linear address from indexes and offset
#define PGADDR(d, t, o) ((uintptr_t)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

// address in page table or page directory entry
#define PTE_ADDR(pte)   ((uintptr_t)(pte) & ~0xFFF)
#define PDE_ADDR(pde)   PTE_ADDR(pde)

/* page directory and page table constants */
#define NPDEENTRY       1024	// page directory entries per page directory
#define NPTEENTRY       1024	// page table entries per page table

#define PGSIZE          4096	// bytes mapped by a page
#define PGSHIFT         12	// log2(PGSIZE)
#define PTSIZE          (PGSIZE * NPTEENTRY)	// bytes mapped by a page directory entry
#define PTSHIFT         22	// log2(PTSIZE)

#define PTXSHIFT        12	// offset of PTX in a linear address
#define PDXSHIFT        22	// offset of PDX in a linear address

/* page table/directory entry flags */
#define PTE_P           0x001	// Present
#define PTE_W           0x002	// Writeable
#define PTE_U           0x004	// User
#define PTE_PWT         0x008	// Write-Through
#define PTE_PCD         0x010	// Cache-Disable
#define PTE_A           0x020	// Accessed
#define PTE_D           0x040	// Dirty
#define PTE_PS          0x080	// Page Size
#define PTE_MBZ         0x180	// Bits must be zero
#define PTE_AVAIL       0xE00	// Available for software use
						// The PTE_AVAIL bits aren't used by the kernel or interpreted by the
						// hardware, so user processes are allowed to set them arbitrarily.

#define PTE_USER        (PTE_U | PTE_W | PTE_P)

#endif /* !__KERN_MM_MMU_H__ */
