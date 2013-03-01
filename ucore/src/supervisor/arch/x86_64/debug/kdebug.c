#include <types.h>
#include <arch.h>
#include <stdio.h>
#include <string.h>
#include <kdebug.h>

#define STACKFRAME_DEPTH 20

/* *
 * print_kerninfo - print the information about kernel, including the location
 * of kernel entry, the start addresses of data and text segements, the start
 * address of free memory and how many memory that kernel has used.
 * */
void print_kerninfo(void)
{
	extern char __etext[], __edata[], __end[], kern_init[];
	cprintf("Special kernel symbols:\n");
	cprintf("  entry  0x%08x (phys)\n", kern_init);
	cprintf("  etext  0x%08x (phys)\n", __etext);
	cprintf("  edata  0x%08x (phys)\n", __edata);
	cprintf("  end    0x%08x (phys)\n", __end);
	cprintf("Kernel executable memory footprint: %dKB\n",
		(__end - kern_init + 1023) / 1024);
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
		cprintf("rbp:0x%016llx rip:0x%016llx\n", rbp, rip);
		rip = ((uint64_t *) rbp)[1];
		rbp = ((uint64_t *) rbp)[0];
	}
}
