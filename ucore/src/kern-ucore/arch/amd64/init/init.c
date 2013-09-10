#include <types.h>
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
#include <ide.h>
#include <fs.h>
#include <swap.h>
#include <proc.h>
#include <sched.h>
#include <kio.h>
#include <mp.h>
#include <mod.h>
#include <percpu.h>
#include <sysconf.h>
#include <dde_kit/dde_kit.h>

int kern_init(void) __attribute__ ((noreturn));

static void bootaps(void)
{
	int i;
	kprintf("starting to boot Application Processors!\n");
	for(i=0;i<sysconf.lcpu_count;i++){
		if(i == myid())
			continue;
		kprintf("booting cpu%d\n", i);
		cpu_up(i);
	}
}

int kern_init(void)
{
	extern char edata[], end[];
	memset(edata, 0, end - edata);

	/* percpu variable for CPU0 is preallocated */
	percpu_offsets[0] = __percpu_start;

	cons_init();		// init the console

	const char *message = "(THU.CST) os is loading ...";
	kprintf("%s\n\n", message);

	print_kerninfo();

	/* get_cpu_var not available before tls_init() */
	tls_init(per_cpu_ptr(cpus, 0));

	pmm_init();		// init physical memory management

	hz_init();

	//init the acpi stuff
	acpitables_init();

	idt_init();		// init interrupt descriptor table
	pic_init();		// init interrupt controller

//	acpi_conf_init();
	lapic_init();
	numa_init();
	percpu_init();
	cpus_init();

	vmm_init();		// init virtual memory management
	sched_init();		// init scheduler
	proc_init();		// init process table
	sync_init();		// init sync struct

	/* ext int */
	ioapic_init();
	acpi_init();

	ide_init();		// init ide devices
#ifdef UCONFIG_SWAP
	swap_init();		// init swap
#endif
	fs_init();		// init fs

	clock_init();		// init clock interrupt
	mod_init();

	trap_init();

	//XXX put here?
	bootaps();

	intr_enable();		// enable irq interrupt

#ifdef UCONFIG_HAVE_LINUX_DDE36_BASE
	dde_kit_init();
#endif

	/* do nothing */
	cpu_idle();		// run idle process
}

void do_halt(void)
{
	acpi_power_off();
	for (;;);
}
