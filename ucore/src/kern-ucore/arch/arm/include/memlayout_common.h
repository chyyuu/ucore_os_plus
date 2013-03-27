/*
 * =====================================================================================
 *
 *       Filename:  memlayout.h
 *
 *    Description:  refer to board memlayout
 *
 *        Version:  1.0
 *        Created:  03/19/2012 11:25:31 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef __KERN_MACH_MEMLAYOUT_H__
#define __KERN_MACH_MEMLAYOUT_H__

#ifndef __ASSEMBLER__

#include <types.h>
#include <atomic.h>
#include <list.h>
#include <mmu.h>

/* Memory map */
#define MEMMAPMAX			20	// number of entries
/* Type */
#define MEMMAP_MEMORY 1		// address range memory
#define MEMMAP_RESERVED 2	// address range reserved

/* This must NOT be 0 */
#define PBASE               (KERNBASE+0x100)

/* Might need to fusion with Region struct of MMU */
struct memmap {
	uint32_t nr_map;
	struct {
		uint32_t addr;
		uint32_t size;
		uint32_t type;
	} map[MEMMAPMAX];
};

struct Page;

#ifdef UCONFIG_BIONIC_LIBC
struct mapped_addr {
	struct Page *page;
	off_t offset;
	list_entry_t list;
};

#define le2maddr(le)	\
		to_struct((le), struct mapped_addr, list)

#endif //UCONFIG_BIONIC_LIBC

/* *
 * struct Page - Page descriptor structures. Each Page describes one
 * physical page (1MB). In kern/mm/pmm.h, you can find lots of useful functions
 * that convert Page to other data types, such as phyical address.
 * */
struct Page {
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
#define PG_IO                      6	//dma page, never free in unmap_page

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
#define SetPageIO(page)         set_bit(PG_IO, &((page)->flags))
#define ClearPageIO(page)       clear_bit(PG_IO, &((page)->flags))
#define PageIO(page)            test_bit(PG_IO, &((page)->flags))

// convert list entry to page
#define le2page(le, member)                 \
    to_struct((le), struct Page, member)

/* free_area_t - maintains a doubly linked list to record free (unused) pages */
typedef struct {
	list_entry_t free_list;	// the list header
	unsigned int nr_free;	// # of free pages in this free list
} free_area_t;

#endif /* !__ASSEMBLER__ */

#endif
