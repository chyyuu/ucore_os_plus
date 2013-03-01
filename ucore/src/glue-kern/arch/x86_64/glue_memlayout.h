#ifndef __GLUE_MEMLAYOUT_H__
#define __GLUE_MEMLAYOUT_H__

#include "glue_mmu.h"

/* global segment number */
#define SEG_KTEXT   1
#define SEG_KDATA   2
#define SEG_KPLS    3
#define SEG_UTEXT   4
#define SEG_UDATA   5
#define SEG_TLS1    6
#define SEG_TLS2	7
#define SEG_TSS     8

/* global descrptor numbers */
#define GD_KTEXT    ((SEG_KTEXT) << 4)	// kernel text
#define GD_KDATA    ((SEG_KDATA) << 4)	// kernel data
#define GD_UTEXT    ((SEG_UTEXT) << 4)	// user text
#define GD_UDATA    ((SEG_UDATA) << 4)	// user data
#define GD_TLS1     ((SEG_TLS1) << 4)
#define GD_TLS2     ((SEG_TLS2) << 4)

#define DPL_KERNEL  (0)
#define DPL_USER    (3)

#define KERNEL_CS   ((GD_KTEXT) | DPL_KERNEL)
#define KERNEL_DS   ((GD_KDATA) | DPL_KERNEL)
#define USER_CS     ((GD_UTEXT) | DPL_USER)
#define USER_DS     ((GD_UDATA) | DPL_USER)

/* All physical memory mapped at this address */
#define SVBASE           0xFFFF800000000000

#define PBASE            0xFFFF900000000000
#define PSIZE            0x0000400000000000	// the maximum amount of physical memory

#define KERNBASE         0xFFFFFFFF80000000

/* *
 * * Virtual page table. Entry PGX[VPT] in the PGD (Page Global Directory) contains
 * a pointer to the page directory itself, thereby turning the PGD into a page
 * table, which maps all the PUEs (Page Upper Directory Table Entry) containing
 * the page mappings for the entire virtual address space into that region starting at VPT.
 * */
#define VPT                 0xFFFFD00000000000
#define VPT_ENTRY(ADDR)     ((pte_t *)(VPT + (((uintptr_t)(ADDR) & 0xFFFFFFFFF000) >> 9)))

#define KSTACKPAGE          4	// # of pages in kernel stack
#define KSTACKSIZE          (KSTACKPAGE * PGSIZE)	// sizeof kernel stack

#define USERTOP             0x0000100000000000
#define USTACKTOP           USERTOP
#define USTACKPAGE          4096	// # of pages in user stack
#define USTACKSIZE          (USTACKPAGE * PGSIZE)	// sizeof user stack

#define USERBASE            0x0000000001000000
#define UTEXT               0x0000000010000000	// where user programs generally begin
#define USTAB               USERBASE	// the location of the user STABS data structure

#define USER_ACCESS(start, end)                     \
    (USERBASE <= (start) && (start) < (end) && (end) <= USERTOP)

#define KERN_ACCESS(start, end)											\
    ((PBASE <= (start) && (start) < (end) && (end) <= PBASE + PSIZE) ||	\
	 ((start) < (end) && KERNBASE <= (start)))

#ifndef __ASSEMBLER__

#include <libs/types.h>

typedef uintptr_t pgd_t;
typedef uintptr_t pud_t;
typedef uintptr_t pmd_t;
typedef uintptr_t pte_t;
typedef pte_t swap_entry_t;

#endif /* !__ASSEMBLER__ */

#endif
