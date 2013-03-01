#include <defs.h>
#include <arch.h>
#include <stdio.h>
#include <string.h>
#include <sync.h>
#include <kdebug.h>
#include <monitor.h>
#include <assert.h>

#define STACKFRAME_DEPTH 20

/* *
 * print_kerninfo - print the information about kernel, including the location
 * of kernel entry, the start addresses of data and text segements, the start
 * address of free memory and how many memory that kernel has used.
 * */
void print_kerninfo(void)
{
	extern char etext[], edata[], end[], kern_init[];
	kprintf("Special kernel symbols:\n");
	kprintf("  entry  0x");
	printhex((unsigned int)kern_init);
	kprintf(" (phys)\n");
	kprintf("  etext\t0x");
	printhex((unsigned int)etext);
	kprintf(" (phys)\n");
	kprintf("  edata\t0x");
	printhex((unsigned int)edata);
	kprintf(" (phys)\n");
	kprintf("  end\t0x");
	printhex((unsigned int)end);
	kprintf(" (phys)\n");
	kprintf("Kernel executable memory footprint: ");
	printbase10((end - etext + 1023) >> 10);
	kprintf("KB\n");
}

/* *
 * print_debuginfo - read and print the stat information for the address @eip,
 * and info.eip_fn_addr should be the first address of the related function.
 * */
void print_debuginfo(uintptr_t eip)
{
	panic("Unimpl");

}

/* *
 * print_stackframe - print a list of the saved eip values from the nested 'call'
 * instructions that led to the current point of execution
 *
 * The x86 stack pointer, namely esp, points to the lowest location on the stack
 * that is currently in use. Everything below that location in stack is free. Pushing
 * a value onto the stack will invole decreasing the stack pointer and then writing
 * the value to the place that stack pointer pointes to. And popping a value do the
 * opposite.
 *
 * The ebp (base pointer) register, in contrast, is associated with the stack
 * primarily by software convention. On entry to a C function, the function's
 * prologue code normally saves the previous function's base pointer by pushing
 * it onto the stack, and then copies the current esp value into ebp for the duration
 * of the function. If all the functions in a program obey this convention,
 * then at any given point during the program's execution, it is possible to trace
 * back through the stack by following the chain of saved ebp pointers and determining
 * exactly what nested sequence of function calls caused this particular point in the
 * program to be reached. This capability can be particularly useful, for example,
 * when a particular function causes an assert failure or panic because bad arguments
 * were passed to it, but you aren't sure who passed the bad arguments. A stack
 * backtrace lets you find the offending function.
 *
 * The inline function read_ebp() can tell us the value of current ebp. And the
 * non-inline function read_eip() is useful, it can read the value of current eip,
 * since while calling this function, read_eip() can read the caller's eip from
 * stack easily.
 *
 * In print_debuginfo(), the function debuginfo_eip() can get enough information about
 * calling-chain. Finally print_stackframe() will trace and print them for debugging.
 *
 * Note that, the length of ebp-chain is limited. In boot/bootasm.S, before jumping
 * to the kernel entry, the value of ebp has been set to zero, that's the boundary.
 * */
void print_stackframe(void)
{
	panic("Unimpl");
}
