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
	extern char __kern_text_start[], __kern_ro_start[], __kern_data_start[];
	extern char edata[], end[], kern_init[];
	kprintf("Special kernel symbols:\n");
	kprintf("  __kern_text_start  %p\n", __kern_text_start);
	kprintf("  __kern_ro_start    %p\n", __kern_ro_start);
	kprintf("  __kern_data_start  %p\n", __kern_data_start);
	kprintf("  edata              %p\n", edata);
	kprintf("  end                %p\n", end);
	kprintf("  kern_init          %p\n", kern_init);
	kprintf("Kernel executable memory footprint: %dKB\n",
		(end - kern_init + 1023) / 1024);
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

	int i, j;
	for (i = 0; rbp != 0 && i < STACKFRAME_DEPTH; i++) {
		kprintf("rbp:%p rip:%p\n", rbp, rip);
		rip = ((uint64_t *) rbp)[1];
		rbp = ((uint64_t *) rbp)[0];
	}
}
