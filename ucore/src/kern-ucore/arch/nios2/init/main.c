#include <system.h>
#include <nios2_irq.h>
#include <nios2_timer.h>
#include <io.h>
#include <console.h>
#include <nios2.h>
#include <clock.h>
#include <assert.h>
#include <proc.h>
#include <stdlib.h>
#include <sched.h>
#include <kio.h>
#include <pmm.h>
#include <vmm.h>
#include <rf212.h>
 int main(void) __attribute__ ((weak, alias("alt_main")));
 extern void swap_init_nios2(void);
 int alt_main(void) 
{
	const char *message = "(THU.CST) os is loading ...";
	kprintf("%s\n\n", message);
	 mp_init();
	 pmm_init();		// init physical memory management    
	pmm_init_ap();
	 vmm_init();		// init virtual memory management
	sched_init();		// init scheduler
	proc_init();		// init process table
	sync_init();
	 ide_init();		// init ide devices
	
	    //swap_init_nios2();
	    fs_init();		// init fs
	irq_init();		// enable irq interrupt
	rf212_init();		// enable rf212 wireless driver
	cons_init();		// init the console, should be after irq_int()!
	clock_init();		// init clock interrupt
	cpu_idle();		// run idle process
	panic("alt_main ends\n");
} 
