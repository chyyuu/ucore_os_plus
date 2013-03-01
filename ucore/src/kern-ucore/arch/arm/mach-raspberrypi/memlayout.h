#ifndef __KERN_MACH_RASPBERRYPI_MEMLAYOUT_H__
#define __KERN_MACH_RASPBERRYPI_MEMLAYOUT_H__

#include <board.h>

/* This file contains the definitions for memory management in our OS. */

/* *
 * Virtual memory map:                                          Permissions
 *                                                              kernel/user
 *
 *     4G ------------------> +---------------------------------+ 0xFFFFFFFF
 *                            |                                 |
 *                            |         Empty Memory (*)        |
 *                            |                                 |
 *                            +---------------------------------+ 0xE4000000
 *                            |   Virt. Page Table (Kern, RW)   | RW/-- 4 Mb
 *     VPT -----------------> +---------------------------------+ 0xE0000000
 *                            |                                 |
 *                            |                                 |
 *     USERTOP -------------> +---------------------------------+ 0xB0000000 -----------------------
 *                            |           User stack            |
 *                            +---------------------------------+
 *                            |                                 |
 *                            :                                 :
 *                            |         ~~~~~~~~~~~~~~~~        |
 *                            :                                 :
 *                            |                                 |
 *                            ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *                            |       User Program & Heap       |
 *     UTEXT ---------------> +---------------------------------+ 0x00800000
 *                            |        Invalid Memory (*)       | --/--
 *                            |  - - - - - - - - - - - - - - -  |
 *                            |    User STAB Data (optional)    |
 *     USERBASE, USTAB------> +---------------------------------+ 0x70000000 -----------------------
 *                            |                                 |
 *                            |            Ramdisk              |
 *                            |                                 |
 *     PERIPHTOP -----------> +---------------------------------+ 0x60000000
 *                            |                                 |
 *                            |     Fixed address, no cache     | RW/--
 *                            |                                 |
 *     PERIPHBASE ----------> +---------------------------------+ 0x48000000
 *                            |                                 |
 *                            |        Invalid Memory (*)       | --/--
 *                            |                                 |
 *     KERNTOP -------------> +---------------------------------+ 0x40000000 ----------------------
 *                            |                                 |
 *                            |         Fixed address           |
 *                            |                                 |
 *             -------------> +---------------------------------+ 0x30800000
 *                            |             Stack               |
 *                            +---------------------------------+                         KMEMSIZE
 *                            |                                 |
 *             -------------> +---------------------------------+ 0x30600000
 *                            |                                 |
 *                            |         Fixed address           | RW/--
 *                            |                                 |
 *     KERNBASE ------------> +---------------------------------+ 0x30000000 -----------------------
 *                            |                                 |
 *                            |                                 |
 *                            |        Invalid Memory (*)       | --/--
 *                            |                                 |
 *     0  ------------------> ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * (*) Note: The kernel ensures that "Invalid Memory" is *never* mapped.
 *     "Empty Memory" is normally unmapped, but user programs may map pages
 *     there if desired.
 *
 * */

/* All physical memory mapped at this address */
#define KERNBASE            SDRAM0_START
#define KMEMSIZE            SDRAM0_SIZE	// the maximum amount of physical memory
#define KERNTOP             (KERNBASE + KMEMSIZE)

/* IO addr space */
#define KIOBASE IO_SPACE_START
#define KIOSIZE IO_SPACE_SIZE

/* end IO */

/* *
 * Virtual page table. Entry PDX[VPT] in the PD (Page Directory) contains
 * a pointer to the page directory itself, thereby turning the PD into a page
 * table, which maps all the PTEs (Page Table Entry) containing the page mappings
 * for the entire virtual address space into that 4 Meg region starting at VPT.
 * */
#define VPT_BASE                 0xD0000000	//KERNBASE + 0x600000

/* For check routines */
//#define TEST_PAGE 0xD0000000

#define KSTACKPAGE          2	// # of pages in kernel stack
#define KSTACKSIZE          (KSTACKPAGE * PGSIZE)	// sizeof kernel stack

#define USERTOP             0x70000000
#define USTACKTOP           USERTOP
#define USTACKPAGE          256	// # of pages in user stack
#define USTACKSIZE          (USTACKPAGE * PGSIZE)	// sizeof user stack

#define USERBASE            0x30000000
#define UTEXT               0x30800000	// where user programs generally begin
#define USTAB               USERBASE	// the location of the user STABS data structure

#define USER_ACCESS(start, end)                     \
    (USERBASE <= (start) && (start) < (end) && (end) <= USERTOP)

#define KERN_ACCESS(start, end)                     \
    (KERNBASE <= (start) && (start) < (end) && (end) <= KERNTOP)

#ifdef UCONFIG_HAVE_RAMDISK
#define DISK_FS_VBASE             0xB0000000
#endif
#ifdef HAS_SHARED_KERNMEM
#define SHARED_KERNMEM_VBASE 0xD0008000
#define SHARED_KERNMEM_PAGES 256	/* 1M */
#endif

#define UCORE_IOREMAP_BASE 0xEF000000
#define UCORE_IOREMAP_SIZE 0x01000000
#define UCORE_IOREMAP_END  (UCORE_IOREMAP_BASE+UCORE_IOREMAP_SIZE)

#ifndef __NO_UCORE_TYPE__
#include <memlayout_common.h>
#endif

#endif /* !__KERN_MACH_RASPBERRYPI_MEMLAYOUT_H__ */
