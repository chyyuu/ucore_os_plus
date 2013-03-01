/*
 * Most parts of this file is copied from the original edition.
 *     The page-related parts are obviously architecture-independent.
 * Should consider merging them into a common header in the future.
 */

#ifndef __ARCH_UM_INCLUDE_MEMLAYOUT_H__
#define __ARCH_UM_INCLUDE_MEMLAYOUT_H__

#include <mmu.h>

/* 
 * Memory mapping
 *
 *      +------------------------------+
 *		|                              |
 *		|       Host Kernel Area       |
 *		|                              |
 *		+------------------------------+ <---------- HOSTBASE
 *		|        ** Invalid **         |
 *		+------------------------------+ <---------- KERNTOP
 *		|                              |
 *		|    Mapped Physical Memory    |
 *		|                              |
 *		+------------------------------+ <---------- KERNBASE / USERTOP
 *		|                              |
 *		:                              :
 *		|       ~~~~~~~~~~~~~~~        |
 *		:                              :
 *		|                              |
 *		+------------------------------+ <---------- USERBASE
 *		|        ** Invalid **         |
 *		+------------------------------+ <---------- 0
 *
 * Details of 'Physical Memory'
 *
 *    +------------------------------+
 *	  |                              |
 *	  |         Free Memory          |
 *	  |                              |
 *	  +------------------------------+
 *	  |                              |
 *	  |      Kernel Code & Data      |   +--- +------------------------------+ <----- RESERVED_END
 *	  |                              |   |	  |   Boot code & Kernel stack   |
 *	  +------------------------------+ --+	  +------------------------------+ <----- BOOT_CODE
 *	  |                              |		  |                              |
 *	  |           Reserved           |		  |     Not used at present      |
 *	  |                              |		  |                              |
 *	  +------------------------------+ --+    +------------------------------+
 * 	  								     |	  |      Global Infomation       |
 *										 +--- +------------------------------+ <----- KERNBASE / GLOBL_INFO
 *	
 *
 */

#define KERNBASE           0xB0000000	/* this is the lowest address we can map */
#define PBASE              KERNBASE
#define KMEMSIZE           0x8000000	/* the maximum amount of physical memory */
#define KERNTOP            (KERNBASE + KMEMSIZE)
#define HOSTBASE           0xC0000000

#define RESERVED_SIZE      0x100000
#define RESERVED_END       (KERNBASE + RESERVED_SIZE)
#define BOOT_CODE          (KERNBASE + 0xFF000)

#define USERBASE           0x00200000
#define UTEXT              0x00800000	// where user programs generally begin
#define USTAB              USERBASE	// the location of the user STABS data structure
#define STUB_START         0x00100000
#define STUB_CODE          STUB_START
#define STUB_DATA          (STUB_CODE + PGSIZE)
#define STUB_END           (STUB_DATA + PGSIZE)

#define USERTOP            0xB0000000
#define USTACKTOP          USERTOP
#define USTACKPAGE         256	// # of pages in user stack
#define USTACKSIZE         (USTACKPAGE * PGSIZE)	// sizeof user stack

#define KSTACKPAGE         2	// # of pages in kernel stack
#define KSTACKSIZE         (KSTACKPAGE * PGSIZE)	// sizeof kernel stack
#define KSTACKTOP          (RESERVED_END)
#define KSTACK             (KSTACKTOP - KSTACKSIZE)

#define USER_ACCESS(start, end)                     \
    (USERBASE <= (start) && (start) < (end) && (end) <= USERTOP)

#define KERN_ACCESS(start, end)                     \
    (KERNBASE <= (start) && (start) < (end) && (end) <= KERNTOP)

#define GLOBL_INFO         KERNBASE

#ifndef __ASSEMBLER__

#ifndef __BOOT__

#include <types.h>
#include <atomic.h>
#include <list.h>

/* Basic types for mm. */
typedef uintptr_t pte_t;
typedef uintptr_t pde_t;
typedef uintptr_t pud_t;
typedef uintptr_t pmd_t;
typedef uintptr_t pgd_t;
typedef pte_t swap_entry_t;

/* *
 * struct Page - Page descriptor structures. Each Page describes one
 * physical page. In pmm.h, you can find lots of useful functions
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
#define PG_reserved					0	// the page descriptor is reserved for kernel or unusable
#define PG_property					1	// the member 'property' is valid
#define PG_slab                     2	// page frame is included in a slab
#define PG_dirty                    3	// the page has been modified
#define PG_swap                     4	// the page is in the active or inactive page list (and swap hash table)
#define PG_active                   5	// the page is in the active page list

#define SetPageReserved(page)		set_bit(PG_reserved, &((page)->flags))
#define ClearPageReserved(page)		clear_bit(PG_reserved, &((page)->flags))
#define PageReserved(page)			test_bit(PG_reserved, &((page)->flags))
#define SetPageProperty(page)		set_bit(PG_property, &((page)->flags))
#define ClearPageProperty(page)		clear_bit(PG_property, &((page)->flags))
#define PageProperty(page)			test_bit(PG_property, &((page)->flags))
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

// convert list entry to page
#define le2page(le, member)					\
	to_struct((le), struct Page, member)

/* free_area_t - maintains a doubly linked list to record free (unused) pages */
typedef struct {
	list_entry_t free_list;	// the list header
	unsigned int nr_free;	// # of free pages in this free list
} free_area_t;

#endif /* !__BOOT__ */

#define E820MAX             20	// number of entries in E820MAP
#define E820_ARM            1	// address range memory
#define E820_ARR            2	// address range reserved

struct E820MAP {
	int nr_map;
	struct {
		uint64_t addr;
		uint64_t size;
		uint32_t type;
	} __attribute__ ((packed)) map[E820MAX];

};

#define STATUS_RUN          0
#define STATUS_DEBUG        1

struct global_info {
	int mem_fd;
	int disk_fd;
	int disk_size;		/* in sector */
	int swap_fd;
	int swap_size;		/* in sector */
	struct E820MAP mem_map;

	int status;
};

#define ginfo              ((struct global_info*)GLOBL_INFO)
#define e820map            (ginfo->mem_map)

#endif /* !__ASSEMBLER__ */

#endif /* !__ARCH_UM_INCLUDE_MEMLAYOUT_H__ */
