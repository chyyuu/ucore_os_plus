#ifndef __KERN_MM_MMU_H__
#define __KERN_MM_MMU_H__

/*  MMU definition for ARM9 */
/*  Chen Yuheng 2012/3/20 */

/* PSR flags */
#define PSR_N 		1 << 31
#define PSR_Z 		1 << 30
#define PSR_C 		1 << 29
#define PSR_V 		1 << 28
#define PSR_I 		0x00000080
#define PSR_F 		0x00000040
#define PSR_T 		0x00000020

/* MMU activation */
#define CHANGEALLDOM 	0xffffffff
#define ENABLEMMU 		0x00000001
#define ENABLEDCACHE 	0x00000004
#define ENABLEICACHE 	0x00001000
#define ENABLEHIGHEVT	0x00002000
// In ARMv6, this enables new page table format
#define ENABLENEWPT   0x00800000
#define CHANGEMMU 		0x00000001
#define CHANGEDCACHE 	0x00000004
#define CHANGEICACHE 	0x00001000
#define CHANGEHIGHEVT	0x00002000
#define CHANGENEWPT   0x00800000
#define ENABLEWB 		0x00000008
#define CHANGEWB 		0x00000008

#define DOM3CLT 		0x00000001	// Critical value

/* type */
#define FAULT 	0
#define COARSE 	1
#define MASTER	2

/* Pagesize in kb */
#define SECTIONPAGE 1024
#define LARGEPAGE 64
#define SMALLPAGE 4

// A linear address 'la' has a three-part structure as follows:
//
// +-------12-------+--------8-------+---------12----------+
// | L1 MasterTable | L2 Page Table  | Offset within Page  |
// |      Index     |     Index      |                     |
// +----------------+----------------+---------------------+
//  \--- PDX(la) --/ \--- PTX(la) --/ \---- PGOFF(la) ----/
//  \----------- PPN(la) -----------/
//
// The PDX, PTX, PGOFF, and PPN macros decompose linear addresses as shown.
// To construct a linear address la from PDX(la), PTX(la), and PGOFF(la),
// use PGADDR(PDX(la), PTX(la), PGOFF(la)).

/* page directory and page table constants */
#define NPDEENTRY       4096	// page directory entries per page directory (L1 MASTER)
#define NPTEENTRY       256	// page table entries per page table (L2 COARSE)

#define PGSIZE          4096	// bytes mapped by a page (L2 SMALLPAGE)
#define PGSHIFT         12	// log2(PGSIZE)
#define PTSIZE          (PGSIZE * NPTEENTRY)	// bytes mapped by a page directory entry (L2 COARSE SMALLPAGE)
#define PMSIZE			PTSIZE
#define PUSIZE			PTSIZE
#define PTSHIFT         20	// log2(PTSIZE)

#define PTXSHIFT        12	// offset of PTX in a linear address
#define PDXSHIFT        20	// offset of PDX in a linear address

#define PMXSHIFT		PDXSHIFT
#define PUXSHIFT		PDXSHIFT
#define PGXSHIFT		PDXSHIFT

// page directory index (L1)
#define PDX(la) ((((uintptr_t)(la)) >> PDXSHIFT) & 0xFFF)
#define PGX(la) PDX(la)
#define PUX(la) PDX(la)
#define PMX(la) PDX(la)

// page table index (L2)
#define PTX(la) ((((uintptr_t)(la)) >> PTXSHIFT) & 0xFF)

// page number field of address
#define PPN(la) (((uintptr_t)(la)) >> PTXSHIFT)

// offset in page
#define PGOFF(la) (((uintptr_t)(la)) & 0xFFF)

// construct linear address from indexes and offset
#define PGADDR(d, t, o) ((uintptr_t)((d) << PDXSHIFT | (t) << PTXSHIFT | (o)))

// address in page table or page directory entry
#define PTE_ADDR(pte)   ((uintptr_t)(pte) & ~0xFFF)
#define PDE_ADDR(pde)   ((uintptr_t)(pde) & ~0x3FF)
#define PMD_ADDR(pmd)   PTE_ADDR(pmd)
#define PUD_ADDR(pud)   PTE_ADDR(pud)
#define PGD_ADDR(pgd)   PTE_ADDR(pgd)

// PTE_xxx are the ucore flags
// PTEX_xxx and PTE_LX_xxx are the hardware flags
#define PTE_STATUS(pte) (pte + 512)

/* page table/directory entry flags used for the bit status PT */
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
#define PTE_IOMEM       0x10000

#define PTE_USER        (PTE_U | PTE_W | PTE_P)

/* page directory (L1) / table (L2) entry flags */
#define PTEX_P 0x3		/* Present (if one bit of [1:0] is set, page exists */
/* PTE type */
/* AP */
/* NA = no access, RO = read only, RW = read/write */
#define PTEX_NANA 0x00
#define PTEX_RWNA 0x01
#define PTEX_RWRO 0x02
#define PTEX_RWRW 0x03
/* CB */
#define PTEX_cb 0x0		/* cb = not cached/not buffered */
#define PTEX_cB 0x1		/* cB = not cached/Buffered */
#define PTEX_WT 0x2		/* WT = Write Through cache */
#define PTEX_WB 0x3		/* WB = write back cache */
/* Typical */
#define PTEX_PWT (PTEX_WT << 2)	// Write Through
#define PTEX_PIO (PTEX_cb << 2)	// Write Through

#ifdef __MACH_ARM_ARMV5
#define PTEX_R   0x000		// Supervisor/Readonly
#define PTEX_W   0x550		// Supervisor/Write
#define PTEX_U   0xAA0		// Supervisor/Write _ User/Read Only
#define PTEX_UW  0xFF0		// Supervisor/Write _ User/Write
#elif defined __MACH_ARM_ARMV6
// see section 5.5.2 and 5.11.2 in ARM11_MPCore_Processor_r2_p0.pdf
#define PTEX_R   0x210		// supervisor ro, user no_access
#define PTEX_W   0x010		// supervisor rw, user no_access
#define PTEX_U   0x020		// supervisor rw, user ro
#define PTEX_UW  0x030		// supervisor rw, user rw
#elif defined __MACH_ARM_ARMV7
#define PTEX_R   0x210		// Supervisor/Readonly
#define PTEX_W   0x010		// Supervisor/Write
#define PTEX_U   0x020		// Supervisor/Write _ User/Read Only
#define PTEX_UW  0x030		// Supervisor/Write _ User/Write
#else
#error Unknown ARM CPU type
#endif

/* Chen Yuheng */
#define PTEX_PROTECT_MASK 0xFF0
#define PTEX_CB_MASK 0xC
#define PTEX_L1_PDTYPE 0x1	//coarse
#define PTEX_L2_PGTYPE 0x2	//small page

#ifndef __ASSEMBLER__

#include <types.h>
/* Page Table Definition */

typedef struct {
	uint32_t vAddress;
	uint32_t ptAddress;
	uint32_t masterPtAddress;
	uint32_t type;
	uint32_t dom;
} Pagetable;

/* Region Descriptor
 * Segment equivalent of x86 */

typedef struct {
	uint32_t vAddress;
	uint32_t pageSize;
	uint32_t numPages;
	uint32_t AP;
	uint32_t CB;
	uint32_t pAddress;
	Pagetable *PT;
} Region;

/* General methods */
//int mmuInitPT(Pagetable *pt);
int mmu_init_pdt(Pagetable * pt);
//int mmuMapRegion(Region *region);
//int mmuAttachPT(Pagetable *pt);

void mmu_init(void);

void tlb_invalidate_all();

extern uintptr_t boot_pgdir_pa;

//XXX
#define PTE_SWAP        (PTE_A | PTE_D)
#define PTE_USER        (PTE_U | PTE_W | PTE_P)

#endif /* !__ASSEMBLER__ */

#endif /* !__KERN_MM_MMU_H__ */
