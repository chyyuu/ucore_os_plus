#include <boot_ap.S.h>
#include <string.h>
#include <pmm.h>
#include <sysconf.h>
#include <lcpu.h>
#include <arch.h>
#include <trap.h>
#include <lapic.h>
#include <memlayout.h>
#include <stdio.h>
#include <driver_os.h>

/* Provided by link script */
extern char boot_ap_entry_64[];

static uintptr_t boot_ap_stack;

int mp_init(void)
{
	memset(VADDR_DIRECT(BOOT_AP_ENTRY), 0, PGSIZE);
	memmove(VADDR_DIRECT(BOOT_AP_ENTRY), boot_ap_entry_64, PGSIZE);

	memset(VADDR_DIRECT(BOOT_AP_CR3), 0, 8);
	memmove(VADDR_DIRECT(BOOT_AP_CR3), &boot_cr3, sizeof(boot_cr3));

	memset(VADDR_DIRECT(BOOT_AP_LAPIC_PHYS), 0, 8);
	memmove(VADDR_DIRECT(BOOT_AP_LAPIC_PHYS), &sysconf.lapic_phys,
		sizeof(sysconf.lapic_phys));

	int i, max_apic = 0;
	for (i = 0; i != sysconf.lcpu_count; ++i) {
		lcpu_static[lcpu_id_set[i]].public.idx = i;
		lcpu_static[lcpu_id_set[i]].public.lapic_id = lcpu_id_set[i];

		lcpu_id_inv[lcpu_id_set[i]] = i;
		if (lcpu_id_set[i] > max_apic)
			max_apic = lcpu_id_set[i];
	}

	boot_ap_stack = (uintptr_t) page2va(alloc_pages(max_apic));
	memset(VADDR_DIRECT(BOOT_AP_STACK_BASE), 0, 8);
	memmove(VADDR_DIRECT(BOOT_AP_STACK_BASE), &boot_ap_stack,
		sizeof(boot_ap_stack));

	int apic_id;
	for (i = 0; i < sysconf.lcpu_count; ++i) {
#if HAS_DRIVER_OS
		if (i == sysconf.lcpu_count - 1)
			break;
#endif
		apic_id = lcpu_id_set[i];
		if (apic_id != sysconf.lcpu_boot) {
			lapic_ap_start(apic_id, BOOT_AP_ENTRY);
		}
	}

#if HAS_DRIVER_OS
	driver_os_init();
#endif

	return 0;
}

/* Copy from pmm.c */
static inline void lgdt(struct pseudodesc *pd)
{
	asm volatile ("lgdt (%0)"::"r" (pd));
	asm volatile ("movw %%ax, %%es"::"a" (KERNEL_DS));
	asm volatile ("movw %%ax, %%ds"::"a" (KERNEL_DS));
	// reload cs & ss
	asm volatile ("movq %%rsp, %%rax;"	// move %rsp to %rax
		      "pushq %1;"	// push %ss
		      "pushq %%rax;"	// push %rsp
		      "pushfq;"	// push %rflags
		      "pushq %0;"	// push %cs
		      "call 1f;"	// push %rip
		      "jmp 2f;"
		      "1: iretq; 2:"::"i" (KERNEL_CS), "i"(KERNEL_DS));
}

void ap_boot_init(void)
{
	lgdt(&gdt_pd);
	ltr(GD_TSS(lapic_id()));
	lidt(&idt_pd);
	lapic_init_ap();

	lcpu_init();
}
