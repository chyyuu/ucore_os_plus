#ifndef _MIPS_VM_H_
#define _MIPS_VM_H_

/*
 * Machine-dependent VM system definitions.
 */

//#define PAGE_SIZE  4096               /* size of VM page */
#define PAGE_FRAME 0xfffff000	/* mask for getting page number from addr */

/*
 * MIPS hardwired memory layout:
 *    0xc0000000 - 0xffffffff   kseg2 (kernel, tlb-mapped)
 *    0xa0000000 - 0xbfffffff   kseg1 (kernel, unmapped, uncached)
 *    0x80000000 - 0x9fffffff   kseg0 (kernel, unmapped, cached)
 *    0x00000000 - 0x7fffffff   kuseg (user, tlb-mapped)
 */

#define MIPS_KUSEG  0x00000000
#define MIPS_KSEG0  0x80000000
#define MIPS_KSEG1  0xa0000000
#define MIPS_KSEG2  0xc0000000

/* 
 * The first 512 megs of physical space can be addressed in both kseg0 and
 * kseg1. We use kseg0 for the kernel. This macro returns the kernel virtual
 * address of a given physical address within that range. (We assume we're
 * not using systems with more physical space than that anyway.)
 *
 * N.B. If you, say, call a function that returns a paddr or 0 on error,
 * check the paddr for being 0 *before* you use this macro. While paddr 0
 * is not legal for memory allocation or memory management (it holds 
 * exception handler code) when converted to a vaddr it's *not* NULL, *is*
 * a valid address, and will make a *huge* mess if you scribble on it.
 */
#define PADDR_TO_KVADDR(paddr) ((paddr)+MIPS_KSEG0)

/*
 * The top of user space. (Actually, the address immediately above the
 * last valid user address.)
 */
//#define USERTOP     MIPS_KSEG0

/*
 * The starting value for the stack pointer at user level.  Because
 * the stack is subtract-then-store, this can start as the next
 * address after the stack area.
 *
 * We put the stack at the very top of user virtual memory because it
 * grows downwards.
 */
//#define USERSTACK   USERTOP

/*
 * Interface to the low-level module that looks after the amount of
 * physical memory we have.
 *
 * ram_getsize returns the lowest valid physical address, and one past
 * the highest valid physical address. (Both are page-aligned.) This
 * is the memory that is available for use during operation, and
 * excludes the memory the kernel is loaded into and memory that is
 * grabbed in the very early stages of bootup.
 *
 * ram_stealmem can be used before ram_getsize is called to allocate
 * memory that cannot be freed later. This is intended for use early
 * in bootup before VM initialization is complete.
 */

void ram_getsize(uintptr_t * lo, uintptr_t * hi);

/*
 * The ELF executable type for this platform.
 */
#define EM_MACHINE  EM_MIPS

#endif /* _MIPS_VM_H_ */
