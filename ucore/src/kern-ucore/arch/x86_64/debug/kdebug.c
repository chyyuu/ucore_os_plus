#include <types.h>
#include <arch.h>
#include <stdio.h>
#include <string.h>
#include <memlayout.h>
#include <vmm.h>
#include <kdebug.h>
#include <kio.h>

#define STACKFRAME_DEPTH 20

/* *
 * print_kerninfo - print the information about kernel, including the location
 * of kernel entry, the start addresses of data and text segements, the start
 * address of free memory and how many memory that kernel has used.
 * */
void print_kerninfo(void)
{
#if 0
	extern char etext[], edata[], end[], kern_init[];
	kprintf("Special kernel symbols:\n");
	kprintf("  entry  0x%08x (phys)\n", kern_init);
	kprintf("  etext  0x%08x (phys)\n", etext);
	kprintf("  edata  0x%08x (phys)\n", edata);
	kprintf("  end    0x%08x (phys)\n", end);
	kprintf("Kernel executable memory footprint: %dKB\n",
		(end - kern_init + 1023) / 1024);
#endif
}

static uint64_t read_rip(void) __attribute__ ((noinline));

static uint64_t read_rip(void)
{
	uint64_t rip;
	asm volatile ("movq 8(%%rbp), %0":"=r" (rip));
	return rip;
}

/* *
 * print_stackframe - print a list of the saved rip values from the nested 'call'
 * instructions that led to the current point of execution
 * */
void print_stackframe(void)
{
	uint64_t rbp = read_rbp(), rip = read_rip();

	int i;
	for (i = 0; rbp != 0 && i < STACKFRAME_DEPTH; i++) {
		kprintf("rbp:0x%016llx rip:0x%016llx\n", rbp, rip);
		rip = ((uint64_t *) rbp)[1];
		rbp = ((uint64_t *) rbp)[0];
	}
}
