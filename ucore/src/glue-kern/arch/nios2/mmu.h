#ifndef __KERN_MM_MMU_H__
#define __KERN_MM_MMU_H__

//#include <types.h>

/* Eflags register */
#define FL_CF           0x00000001	// Carry Flag
#define FL_PF           0x00000004	// Parity Flag
#define FL_AF           0x00000010	// Auxiliary carry Flag
#define FL_ZF           0x00000040	// Zero Flag
#define FL_SF           0x00000080	// Sign Flag
#define FL_TF           0x00000100	// Trap Flag
#define FL_IF           0x00000200	// Interrupt Flag
#define FL_DF           0x00000400	// Direction Flag
#define FL_OF           0x00000800	// Overflow Flag
#define FL_IOPL_MASK    0x00003000	// I/O Privilege Level bitmask
#define FL_IOPL_0       0x00000000	//   IOPL == 0
#define FL_IOPL_1       0x00001000	//   IOPL == 1
#define FL_IOPL_2       0x00002000	//   IOPL == 2
#define FL_IOPL_3       0x00003000	//   IOPL == 3
#define FL_NT           0x00004000	// Nested Task
#define FL_RF           0x00010000	// Resume Flag
#define FL_VM           0x00020000	// Virtual 8086 mode
#define FL_AC           0x00040000	// Alignment Check
#define FL_VIF          0x00080000	// Virtual Interrupt Flag
#define FL_VIP          0x00100000	// Virtual Interrupt Pending
#define FL_ID           0x00200000	// ID flag

/* Application segment type bits */
#define STA_X           0x8	// Executable segment
#define STA_E           0x4	// Expand down (non-executable segments)
#define STA_C           0x4	// Conforming code segment (executable only)
#define STA_W           0x2	// Writeable (non-executable segments)
#define STA_R           0x2	// Readable (executable segments)
#define STA_A           0x1	// Accessed

/* System segment type bits */
#define STS_T16A        0x1	// Available 16-bit TSS
#define STS_LDT         0x2	// Local Descriptor Table
#define STS_T16B        0x3	// Busy 16-bit TSS
#define STS_CG16        0x4	// 16-bit Call Gate
#define STS_TG          0x5	// Task Gate / Coum Transmitions
#define STS_IG16        0x6	// 16-bit Interrupt Gate
#define STS_TG16        0x7	// 16-bit Trap Gate
#define STS_T32A        0x9	// Available 32-bit TSS
#define STS_T32B        0xB	// Busy 32-bit TSS
#define STS_CG32        0xC	// 32-bit Call Gate
#define STS_IG32        0xE	// 32-bit Interrupt Gate
#define STS_TG32        0xF	// 32-bit Trap Gate

#ifdef __ASSEMBLER__

#define SEG_NULL                                                \
    .word 0, 0;                                                 \
    .byte 0, 0, 0, 0

#define SEG_ASM(type,base,lim)                                  \
    .word (((lim) >> 12) & 0xffff), ((base) & 0xffff);          \
    .byte (((base) >> 16) & 0xff), (0x90 | (type)),             \
        (0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)

#else /* not __ASSEMBLER__ */

//#include <defs.h>

#endif /* !__ASSEMBLER__ */

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
#define PGX(la) PDX(la)
#define PUX(la) PDX(la)
#define PMX(la) PDX(la)

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
#define PMD_ADDR(pmd)   PTE_ADDR(pmd)
#define PUD_ADDR(pud)   PTE_ADDR(pud)
#define PGD_ADDR(pgd)   PTE_ADDR(pgd)

/* page directory and page table constants */
#define NPDEENTRY       1024	// page directory entries per page directory
#define NPTEENTRY       1024	// page table entries per page table

#define PGSIZE          4096	// bytes mapped by a page
#define PGSHIFT         12	// log2(PGSIZE)
#define PTSIZE          (PGSIZE * NPTEENTRY)	// bytes mapped by a page directory entry
#define PMSIZE			PTSIZE
#define PUSIZE			PTSIZE
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

#define PTE_SWAP        (PTE_A | PTE_D)
#define PTE_USER        (PTE_U | PTE_W | PTE_P)

/* Control Register flags */
#define CR0_PE          0x00000001	// Protection Enable
#define CR0_MP          0x00000002	// Monitor coProcessor
#define CR0_EM          0x00000004	// Emulation
#define CR0_TS          0x00000008	// Task Switched
#define CR0_ET          0x00000010	// Extension Type
#define CR0_NE          0x00000020	// Numeric Errror
#define CR0_WP          0x00010000	// Write Protect
#define CR0_AM          0x00040000	// Alignment Mask
#define CR0_NW          0x20000000	// Not Writethrough
#define CR0_CD          0x40000000	// Cache Disable
#define CR0_PG          0x80000000	// Paging

#define CR4_PCE         0x00000100	// Performance counter enable
#define CR4_MCE         0x00000040	// Machine Check Enable
#define CR4_PSE         0x00000010	// Page Size Extensions
#define CR4_DE          0x00000008	// Debugging Extensions
#define CR4_TSD         0x00000004	// Time Stamp Disable
#define CR4_PVI         0x00000002	// Protected-Mode Virtual Interrupts
#define CR4_VME         0x00000001	// V86 Mode Extensions

#endif /* !__KERN_MM_MMU_H__ */
