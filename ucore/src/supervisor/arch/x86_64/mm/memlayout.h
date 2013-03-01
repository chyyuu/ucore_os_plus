#ifndef __KERN_MM_MEMLAYOUT_H__
#define __KERN_MM_MEMLAYOUT_H__

/* This file contains the definitions for memory management in our OS. */

/* global segment number */
#define SEG_KTEXT        1
#define SEG_KDATA        2
#define SEG_KPLS         3
#define SEG_UTEXT        4
#define SEG_UDATA        5
#define SEG_TLS1         6
#define SEG_TLS2	     7
#define SEG_TSS(apic_id) 8 + (apic_id)

/* global descrptor numbers */
#define GD_KTEXT    ((SEG_KTEXT) << 4)	// kernel text
#define GD_KDATA    ((SEG_KDATA) << 4)	// kernel data
#define GD_KPLS     ((SEG_KPLS)  << 4)	// kernel pls
#define GD_UTEXT    ((SEG_UTEXT) << 4)	// user text
#define GD_UDATA    ((SEG_UDATA) << 4)	// user data
#define GD_TLS1     ((SEG_TLS1) << 4)
#define GD_TLS2     ((SEG_TLS2) << 4)
#define GD_TSS(apic_id) ((SEG_TSS(apic_id)) << 4)	// task segment selector

#define DPL_KERNEL  (0)
#define DPL_USER    (3)

#define KERNEL_CS   ((GD_KTEXT) | DPL_KERNEL)
#define KERNEL_DS   ((GD_KDATA) | DPL_KERNEL)
#define KERNEL_PLS  ((GD_KPLS)  | DPL_KERNEL)
#define USER_CS     ((GD_UTEXT) | DPL_USER)
#define USER_DS     ((GD_UDATA) | DPL_USER)

/* *
 * Virtual memory map:                                          Permissions
 *                                                              kernel/user
 *
 *     4G x 4G -------------> +---------------------------------+
 *                            |                                 |
 *                            |         Invalid Memory (*)      |
 *                            |                                 |
 *                            +---------------------------------+ 0xFFFFD08000000000
 *                            |   Cur. Page Table (Kern, RW)    | RW/-- PUSIZE
 *     VPT -----------------> +---------------------------------+ 0xFFFFD00000000000
 *                            |                                 |
 *                            |  Linear Mapping of Physical Mem | RW/-- PHYSSIZE
 *                            |                                 |
 *     KERNTOP, PHYSBASE ---> +---------------------------------+ 0xFFFF900000000000
 *                            |                                 |
 *                            |    Remapped Physical Memory     | RW/-- KMEMSIZE
 *                            |                                 |
 *     KERNBASE ------------> +---------------------------------+ 0xFFFF800000000000
 *                            |                                 |
 *                            |                                 |
 *                            |                                 |
 *                            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * (*) Note: The kernel ensures that "Invalid Memory" is *never* mapped.
 *     "Empty Memory" is normally unmapped, but user programs may map pages
 *     there if desired.
 *
 * */

#define SVBASE              0xFFFF800000000000
#define SVSIZE              0x0000100000000000	// the maximum amount of kern remapped memory
#define SVTOP               (SVBASE + SVSIZE)

/* All physical memory mapped at this address */
#define PHYSBASE            0xFFFF900000000000
#define PHYSSIZE            0x0000400000000000	// the maximum amount of physical memory

/* RESERVED MEMORY FOR DRIVER OS */
#define RESERVED_DRIVER_OS_SIZE 0x10000000
/* Communicating buffer size, in pages  */
#define DRIVER_OS_BUFFER_PSIZE  16

/* *
 * * Virtual page table. Entry PGX[VPT] in the PGD (Page Global Directory) contains
 * a pointer to the page directory itself, thereby turning the PGD into a page
 * table, which maps all the PUEs (Page Upper Directory Table Entry) containing
 * the page mappings for the entire virtual address space into that region starting at VPT.
 * */
#define VPT                 0xFFFFD00000000000

#define KSTACKPAGE          4	// # of pages in kernel stack
#define KSTACKSIZE          (KSTACKPAGE * PGSIZE)	// sizeof kernel stack

#ifndef __ASSEMBLER__

#include <types.h>
#include <atomic.h>
#include <list.h>

typedef uintptr_t pgd_t;
typedef uintptr_t pud_t;
typedef uintptr_t pmd_t;
typedef uintptr_t pte_t;

// some constants for bios interrupt 15h AX = 0xE820
#define E820MAX             20	// number of entries in E820MAP
#define E820_ARM            1	// address range memory
#define E820_ARR            2	// address range reserved

struct e820map {
	int nr_map;
	struct {
		uint64_t addr;
		uint64_t size;
		uint32_t type;
	} __attribute__ ((packed)) map[E820MAX];
};

/* *
 * struct Page - Page descriptor structures. Each Page describes one
 * physical page. In kern/mm/pmm.h, you can find lots of useful functions
 * that convert Page to other data types, such as phyical address.
 * */
struct Page {
	atomic_t ref;		// page frame's reference counter
	uint32_t flags;		// array of flags that describe the status of the page frame
	unsigned int property;	// used in buddy system, stores the order (the X in 2^X) of the continuous memory block
	int zone_num;		// used in buddy system, the No. of zone which the page belongs to
	list_entry_t page_link;	// free list link
	void *private;
};

/* Flags describing the status of a page frame */
#define PG_reserved                 0	// the page descriptor is reserved for kernel or unusable
#define PG_property                 1	// the member 'property' is valid

#define SetPageReserved(page)       set_bit(PG_reserved, &((page)->flags))
#define ClearPageReserved(page)     clear_bit(PG_reserved, &((page)->flags))
#define PageReserved(page)          test_bit(PG_reserved, &((page)->flags))
#define SetPageProperty(page)       set_bit(PG_property, &((page)->flags))
#define ClearPageProperty(page)     clear_bit(PG_property, &((page)->flags))
#define PageProperty(page)          test_bit(PG_property, &((page)->flags))

// convert list entry to page
#define le2page(le, member)                 \
    to_struct((le), struct Page, member)

/* free_area_t - maintains a doubly linked list to record free (unused) pages */
typedef struct {
	list_entry_t free_list;	// the list header
	unsigned int nr_free;	// # of free pages in this free list
} free_area_t;

#endif /* !__ASSEMBLER__ */

#endif /* !__KERN_MM_MEMLAYOUT_H__ */
