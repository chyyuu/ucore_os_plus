#ifndef __GLUE_MMU_H__
#define __GLUE_MMU_H__

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

#define SEG_NULL()                              \
    .quad 0x0, 0x0

#define SEG_CODE(type)                          \
    .word 0x0, 0x0;                             \
    .byte 0x0, (0x90 | (type)), 0x20, 0x0;      \
    .word 0x0, 0x0, 0x0, 0x0

#define SEG_DATA(type)                          \
    .word 0x0, 0x0;                             \
    .byte 0x0, (0x90 | (type)), 0x0, 0x0;       \
    .word 0x0, 0x0, 0x0, 0x0

#else /* not __ASSEMBLER__ */

#include <libs/types.h>

/* Segment descriptors for interrupts and traps */
struct gatedesc {
	unsigned int gd_off_15_0:16;	// [0 ~ 16] bits of offset in segment
	unsigned int gd_ss:16;	// segment selector
	unsigned int gd_ist:3;	// interrupt stack table
	unsigned int gd_rsv1:5;	// reserved bits
	unsigned int gd_type:4;	// type (STS_{TG,IG32,TG32})
	unsigned int gd_s:1;	// 0 for system, 1 for code or data
	unsigned int gd_dpl:2;	// descriptor(meaning new) privilege level
	unsigned int gd_p:1;	// Present
	unsigned int gd_off_31_16:16;	// [16 ~ 31] bits of offset in segment
	unsigned int gd_off_63_32:32;	// [32 ~ 63] bits of offset in segment
	unsigned int gd_rsv2:32;	// reserved bits
};

/* *
 * Set up a normal interrupt/trap gate descriptor
 *   - istrap: 1 for a trap (= exception) gate, 0 for an interrupt gate
 *   - sel: Code segment selector for interrupt/trap handler
 *   - off: Offset in code segment for interrupt/trap handler
 *   - dpl: Descriptor Privilege Level - the privilege level required
 *          for software to invoke this interrupt/trap gate explicitly
 *          using an int instruction.
 * */

#define SETGATE(gate, istrap, sel, off, dpl) {              \
        (gate).gd_off_15_0 = (uint64_t)(off) & 0xffff;      \
        (gate).gd_ss = (sel);                               \
        (gate).gd_ist = 0;                                  \
        (gate).gd_rsv1 = 0;                                 \
        (gate).gd_type = (istrap) ? STS_TG32 : STS_IG32;    \
        (gate).gd_s = 0;                                    \
        (gate).gd_dpl = (dpl);                              \
        (gate).gd_p = 1;                                    \
        (gate).gd_off_31_16 = (uint64_t)(off) >> 16;        \
        (gate).gd_off_63_32 = (uint64_t)(off) >> 32;        \
        (gate).gd_rsv2 = 0;                                 \
    }

/* Set up a call gate descriptor */
#define SETCALLGATE(gate, ss, off, dpl) {                   \
        (gate).gd_off_15_0 = (uint64_t)(off) & 0xffff;      \
        (gate).gd_ss = (ss);                                \
        (gate).gd_ist = 0;                                  \
        (gate).gd_rsv1 = 0;                                 \
        (gate).gd_type = STS_CG32;                          \
        (gate).gd_s = 0;                                    \
        (gate).gd_dpl = (dpl);                              \
        (gate).gd_p = 1;                                    \
        (gate).gd_off_31_16 = (uint64_t)(off) >> 16;        \
        (gate).gd_off_63_32 = (uint64_t)(off) >> 32;        \
        (gate).gd_rsv2 = 0;                                 \
    }

/* segment descriptors */
struct segdesc {
	unsigned int sd_lim_15_0:16;	// [0 ~ 15] bits of segment limit
	unsigned int sd_base_15_0:16;	// [0 ~ 15] bits of segment base address
	unsigned int sd_base_23_16:8;	// [16 ~ 23] bits of segment base address
	unsigned int sd_type:4;	// segment type (see STS_ constants)
	unsigned int sd_s:1;	// 0 = system, 1 = application
	unsigned int sd_dpl:2;	// descriptor Privilege Level
	unsigned int sd_p:1;	// present
	unsigned int sd_lim_19_16:4;	// [16 ~ 19] bits of segment limit
	unsigned int sd_avl:1;	// unused (available for software use)
	unsigned int sd_l:1;	// 64-bit code segment
	unsigned int sd_db:1;	// 0 = 16-bit segment, 1 = 32-bit segment
	unsigned int sd_g:1;	// granularity: limit scaled by 4K when set
	unsigned int sd_base_31_24:8;	// [24 ~ 31] bits of segment base address
	unsigned int sd_base_63_32:32;	// [32 ~ 63] bits of segment base address
	unsigned int sd_rsv:32;	// reserved
};

#define SEG_NULL                                            \
    (struct segdesc) {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}

#define SEG(type, dpl)                                      \
    (struct segdesc) {                                      \
        0, 0, 0, type, 1, dpl, 1,                           \
        0, 0, 1, 0, 1, 0, 0, 0                              \
    }

#define SEGTSS(type, base, lim, dpl)                        \
    (struct segdesc) {                                      \
        (lim) & 0xffff, (base) & 0xffff,                    \
        ((base) >> 16) & 0xff, type, 0, dpl, 1,             \
        ((lim) >> 16) & 0xf, 0, 0, 0, 0,                    \
        ((base) >> 24) & 0xff,                              \
        ((base) >> 32) & 0xffffffff, 0                      \
    }

/* task state segment format (as described by the x86_64 architecture book) */
struct taskstate {
	uint32_t ts_rsv0;	// reserved bits
	uint64_t ts_rsp0;	// stack pointers
	uint64_t ts_rsp1;
	uint64_t ts_rsp2;
	uint64_t ts_rsv1;
	uint64_t ts_ist1;	// interrupt stack table (IST) pointers
	uint64_t ts_ist2;
	uint64_t ts_ist3;
	uint64_t ts_ist4;
	uint64_t ts_ist5;
	uint64_t ts_ist6;
	uint64_t ts_ist7;
	uint64_t ts_rsv2;
	uint16_t ts_rsv3;
	uint16_t ts_iomb;	// i/o map base address
} __attribute__ ((packed));

#endif /* ! __ASSEMBLER__ */

// A linear address 'la' has a three-part structure as follows:
// PGD : Page Global Directory
// PUD : Page Upper Directory
// PMD : Page Mid-level Directory
// PTE : Page Table Entry
//
// +------16-------+-----9-----+-----9-----+-----9-----+-----9-----+-----12-----+
// | sign extended | PGD Index | PUD Index | PMD Index | PTE Index | PTE Offset |
// +---------------+-----------+-----------+-----------+-----------+------------+
//                  \-- PGX --/ \-- PUX --/ \-- PMX --/ \-- PTX --/ \-- PGOFF --/
//                  \------------------- PPN(la) -----------------/
//
// The PGX, PUX, PTX, PGOFF, and PPN macros decompose linear addresses as shown.
// To construct a linear address la from PGX(la), PUX(la), PMX(la), PMX(la), PTX(la),
// and PGOFF(la), use PGADDR(PGX(la), PUX(la), PMX(la), PTX(la), PGOFF(la)).

// page directory index
#define PGX(la) ((((uintptr_t)(la)) >> PGXSHIFT) & 0x1FF)
#define PUX(la) ((((uintptr_t)(la)) >> PUXSHIFT) & 0x1FF)
#define PMX(la) ((((uintptr_t)(la)) >> PMXSHIFT) & 0x1FF)

// page table index
#define PTX(la) ((((uintptr_t)(la)) >> PTXSHIFT) & 0x1FF)

// page number field of address
#define PPN(la) (((uintptr_t)(la)) >> PTXSHIFT)

// offset in page
#define PGOFF(la) (((uintptr_t)(la)) & 0xFFF)

// construct linear address from indexes and offset
#define PGADDR(g, u, m, t, o)                                           \
    ((uintptr_t)(((g) & 0x100) ? ((g) | ~0x1FF) : (g)) << PGXSHIFT      \
     | (uintptr_t)(u) << PUXSHIFT | (uintptr_t)(m) << PMXSHIFT          \
     | (uintptr_t)(t) << PTXSHIFT | (o))

// address in page table or page directory entry
#define PTE_ADDR(pte)   ((uintptr_t)(pte) & ~0xFFF)
#define PMD_ADDR(pmd)   PTE_ADDR(pmd)
#define PUD_ADDR(pud)   PTE_ADDR(pud)
#define PGD_ADDR(pgd)   PTE_ADDR(pgd)

/* page directory and page table constants */
#define NPGENTRY        512	// #entries per page directory

#define PGSIZE          4096	// bytes mapped by a page
#define PGSHIFT         12	// log2(PGSIZE)

#define PTSIZE         (1LLU * NPGENTRY * PGSIZE)	// bytes mapped by a pmd entry
#define PMSIZE         (1LLU * NPGENTRY * PTSIZE)	// bytes mapped by a pud entry
#define PUSIZE         (1LLU * NPGENTRY * PMSIZE)	// bytes mapped by a pgd entry

#define PTXSHIFT        12	// offset of PTX in a linear address
#define PMXSHIFT        21	// offset of PMX in a linear address
#define PUXSHIFT        30	// offset of PUX in a linear address
#define PGXSHIFT        39	// offset of PGX in a linear address

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
#define CR4_PGE         0x00000080	// Page Global Enable
#define CR4_MCE         0x00000040	// Machine Check Enable
#define CR4_PAE         0x00000020	// Physical Address Extension
#define CR4_PSE         0x00000010	// Page Size Extensions
#define CR4_DE          0x00000008	// Debugging Extensions
#define CR4_TSD         0x00000004	// Time Stamp Disable
#define CR4_PVI         0x00000002	// Protected-Mode Virtual Interrupts
#define CR4_VME         0x00000001	// V86 Mode Extensions

#ifndef __ASSEMBLER__

typedef uintptr_t pgd_t;
typedef uintptr_t pud_t;
typedef uintptr_t pmd_t;
typedef uintptr_t pte_t;
typedef pte_t swap_entry_t;
typedef pte_t pte_perm_t;

static inline void ptep_map(pte_t * ptep, uintptr_t pa)
{
	*ptep = (pa | PTE_P);
}

static inline void ptep_unmap(pte_t * ptep)
{
	*ptep = 0;
}

static inline int ptep_invalid(pte_t * ptep)
{
	return (*ptep == 0);
}

static inline int ptep_present(pte_t * ptep)
{
	return (*ptep & PTE_P);
}

static inline int ptep_s_read(pte_t * ptep)
{
	return (*ptep & PTE_P);
}

static inline int ptep_s_write(pte_t * ptep)
{
	return (*ptep & PTE_W);
}

static inline int ptep_u_read(pte_t * ptep)
{
	return (*ptep & PTE_U);
}

static inline int ptep_u_write(pte_t * ptep)
{
	return ((*ptep & PTE_U) && (*ptep & PTE_W));
}

static inline void ptep_set_s_read(pte_t * ptep)
{
}

static inline void ptep_set_s_write(pte_t * ptep)
{
}

static inline void ptep_set_u_read(pte_t * ptep)
{
	*ptep |= PTE_U;
}

static inline void ptep_set_u_write(pte_t * ptep)
{
	*ptep |= PTE_W | PTE_U;
}

static inline void ptep_unset_s_read(pte_t * ptep)
{
}

static inline void ptep_unset_s_write(pte_t * ptep)
{
	*ptep &= (~PTE_W);
}

static inline void ptep_unset_u_read(pte_t * ptep)
{
}

static inline void ptep_unset_u_write(pte_t * ptep)
{
	*ptep &= (~PTE_W);
}

static inline pte_perm_t ptep_get_perm(pte_t * ptep, pte_perm_t perm)
{
	return (*ptep) & perm;
}

static inline void ptep_set_perm(pte_t * ptep, pte_perm_t perm)
{
	*ptep |= perm;
}

static inline void ptep_copy(pte_t * to, pte_t * from)
{
	*to = *from;
}

static inline void ptep_unset_perm(pte_t * ptep, pte_perm_t perm)
{
	*ptep &= (~perm);
}

static inline int ptep_accessed(pte_t * ptep)
{
	return *ptep & PTE_A;
}

static inline int ptep_dirty(pte_t * ptep)
{
	return *ptep & PTE_D;
}

static inline void ptep_set_accessed(pte_t * ptep)
{
	*ptep |= PTE_A;
}

static inline void ptep_set_dirty(pte_t * ptep)
{
	*ptep |= PTE_D;
}

static inline void ptep_unset_accessed(pte_t * ptep)
{
	*ptep &= (~PTE_A);
}

static inline void ptep_unset_dirty(pte_t * ptep)
{
	*ptep &= (~PTE_D);
}

#endif

#endif /* !__KERN_MM_MMU_H__ */
