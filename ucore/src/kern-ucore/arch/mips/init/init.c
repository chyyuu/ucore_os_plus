#include <defs.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <kdebug.h>
#include <picirq.h>
#include <trap.h>
#include <clock.h>
#include <intr.h>
#include <pmm.h>
#include <vmm.h>
#include <proc.h>
#include <thumips_tlb.h>
#include <sched.h>

void setup_exception_vector()
{
	//for QEMU sim
	extern unsigned char __exception_vector, __exception_vector_end;
	memcpy((unsigned int *)0xBFC00000, &__exception_vector,
	       &__exception_vector_end - &__exception_vector);
}

void __noreturn kern_init(void)
{
	//setup_exception_vector();
	tlb_invalidate_all();
	char *p = 0x7ffff000;

	mp_init();
	pic_init();		// init interrupt controller
	cons_init();		// init the console
	clock_init();		// init clock interrupt

	check_initrd();

	const char *message = "(THU.CST) os is loading ...\n\n";
	kprintf(message);
	print_kerninfo();

#if 0
	kprintf("EX\n");
	__asm__ volatile ("syscall");
	kprintf("EX RET\n");
#endif

	pmm_init();		// init physical memory management

	vmm_init();		// init virtual memory management

	sched_init();
	proc_init();		// init process table
	ide_init();
	fs_init();
	intr_enable();		// enable irq interrupt
	//*(int*)(0x00124) = 0x432;
	//asm volatile("divu $1, $1, $1");
//*p = 1;
	cpu_idle();
}
