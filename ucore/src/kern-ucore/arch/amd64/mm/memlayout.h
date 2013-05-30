#ifndef __GLUE_UCORE_MEMLAYOUT_H__
#define __GLUE_UCORE_MEMLAYOUT_H__

/* This file contains the definitions for memory management in our OS. */

/* global segment number */
#define SEG_KTEXT   1
#define SEG_KDATA   2
//#define SEG_KPLS    3
#define SEG_UTEXT   3
#define SEG_UDATA   4
//#define SEG_TLS1    6
//#define SEG_TLS2    7
#define SEG_TSS     5
#define SEG_COUNT  (SEG_TSS+1)

/* global descrptor numbers */
#define GD_KTEXT    ((SEG_KTEXT) << 4)	// kernel text
#define GD_KDATA    ((SEG_KDATA) << 4)	// kernel data
#define GD_UTEXT    ((SEG_UTEXT) << 4)	// user text
#define GD_UDATA    ((SEG_UDATA) << 4)	// user data
//#define GD_TLS1     ((SEG_TLS1) << 4)
//#define GD_TLS2     ((SEG_TLS2) << 4)
#define GD_TSS      ((SEG_TSS) << 4)	// task segment selector

#define DPL_KERNEL  (0)
#define DPL_USER    (3)

#define KERNEL_CS   ((GD_KTEXT) | DPL_KERNEL)
#define KERNEL_DS   ((GD_KDATA) | DPL_KERNEL)
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

/* All physical memory mapped at this address */
#define KERNBASE         0xFFFF800000000000
#define PBASE            KERNBASE
#define KMEMSIZE         0x0000400000000000	// the maximum amount of physical memory
#define KERNTOP          (KERNBASE + KMEMSIZE)

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

#define KERN_ACCESS(start, end)						\
	(PBASE <= (start) && (start) < (end) && (end) <= KERNTOP)

#ifndef __ASSEMBLER__

#include <libs/types.h>
#include <list.h>
#include <atomic.h>
#include <mmu.h>

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
	uintptr_t pa;
	atomic_t ref;		// page frame's reference counter
	uint32_t flags;		// array of flags that describe the status of the page frame
	unsigned int property;	// used in buddy system, stores the order (the X in 2^X) of the continuous memory block
	int zone_num;		// used in buddy system, the No. of zone which the page belongs to
	list_entry_t page_link;	// free list link
	swap_entry_t index;	// stores a swapped-out page identifier
	list_entry_t swap_link;	// swap hash link
};

/* Flags describing the status of a page frame */
#define PG_reserved                 0	// the page descriptor is reserved for kernel or unusable
#define PG_property                 1	// the member 'property' is valid
#define PG_slab                     2	// page frame is included in a slab
#define PG_dirty                    3	// the page has been modified
#define PG_swap                     4	// the page is in the active or inactive page list (and swap hash table)
#define PG_active                   5	// the page is in the active page list
#define PG_IO                       6	// dma page, never free in unmap_page

#define SetPageReserved(page)       set_bit(PG_reserved, &((page)->flags))
#define ClearPageReserved(page)     clear_bit(PG_reserved, &((page)->flags))
#define PageReserved(page)          test_bit(PG_reserved, &((page)->flags))
#define SetPageProperty(page)       set_bit(PG_property, &((page)->flags))
#define ClearPageProperty(page)     clear_bit(PG_property, &((page)->flags))
#define PageProperty(page)          test_bit(PG_property, &((page)->flags))
#define SetPageSlab(page)           set_bit(PG_slab, &((page)->flags))
#define ClearPageSlab(page)         clear_bit(PG_slab, &((page)->flags))
#define PageSlab(page)              test_bit(PG_slab, &((page)->flags))
#define SetPageDirty(page)          set_bit(PG_dirty, &((page)->flags))
#define ClearPageDirty(page)        clear_bit(PG_dirty, &((page)->flags))
#define PageDirty(page)             test_bit(PG_dirty, &((page)->flags))
#define SetPageSwap(page)           set_bit(PG_swap, &((page)->flags))
#define ClearPageSwap(page)         clear_bit(PG_swap, &((page)->flags))
#define PageSwap(page)              test_bit(PG_swap, &((page)->flags))
#define SetPageActive(page)         set_bit(PG_active, &((page)->flags))
#define ClearPageActive(page)       clear_bit(PG_active, &((page)->flags))
#define PageActive(page)            test_bit(PG_active, &((page)->flags))
#define SetPageIO(page)             set_bit(PG_IO, &((page)->flags))
#define ClearPageIO(page)           clear_bit(PG_IO, &((page)->flags))
#define PageIO(page)                test_bit(PG_IO, &((page)->flags))

// convert list entry to page
#define le2page(le, member)                 \
    to_struct((le), struct Page, member)
#endif /* !__ASSEMBLER__ */

#endif
