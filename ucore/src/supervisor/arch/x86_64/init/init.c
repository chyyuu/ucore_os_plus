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
#include <ioapic.h>
#include <lapic.h>
#include <acpi_conf.h>
#include <sysconf.h>
#include <arch.h>
#include <lcpu.h>
#include <cpuid.h>
#include <context.h>
#include <ide.h>
#include <kern.h>
#include <mp.h>

void kern_init(void) __attribute__ ((noreturn));

void kern_init(void)
{
	extern char __edata[], __end[];
	memset(__edata, 0, __end - __edata);

	cons_init();		// init the console

	const char *message = "(THU.CST) os is loading ...";
	cprintf("%s\n\n", message);

	print_kerninfo();

	/* Get the self apic id for locating the TSS */
	uint32_t b;
	cpuid(1, NULL, &b, NULL, NULL);
	int cur_apic_id = (b >> 24) & 0xff;
	sysconf.lcpu_boot = cur_apic_id;

	pmm_init();		// init physical memory management

	pic_init();		// init interrupt controller
	idt_init();		// init interrupt descriptor table

	context_init();

	acpi_conf_init();
	lapic_init();
	ioapic_init();

	ide_init();
	load_kern();

	mp_init();

	lcpu_init();
}
