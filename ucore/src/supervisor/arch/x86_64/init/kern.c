#include <kern.h>
#include <ide.h>
#include <pmm.h>
#include <mmu.h>
#include <lapic.h>
#include <ioapic.h>
#include <lcpu.h>
#include <arch.h>
#include <spinlock.h>
#include <stdio.h>
#include <sysconf.h>
#include <string.h>
#include <console.h>
#include <driver_os.h>

kern_bootinfo_s kern_bootinfo;
char *kern_data;
uintptr_t kern_share;

static void
read_secs(unsigned short ideno, uint32_t secno, void *dst, size_t nsecs)
{
	while (nsecs > 0) {
		size_t curn = nsecs > 64 ? 64 : nsecs;
		ide_read_secs(ideno, secno, dst, curn);

		nsecs -= curn;
		secno += curn;
		dst = (char *)dst + curn * SECTSIZE;
	}
}

void load_kern(void)
{
	ide_read_secs(0, 1, &kern_bootinfo, PGSIZE / SECTSIZE);
	uintptr_t kern_data_size =
	    kern_bootinfo.kern_bss - kern_bootinfo.kern_text;
	kern_data = page2va(alloc_pages(kern_data_size / PGSIZE));
	read_secs(0,
		  1 + (kern_bootinfo.kern_text -
		       kern_bootinfo.kern_start) / SECTSIZE, kern_data,
		  kern_data_size / SECTSIZE);

	kern_share =
	    page2pa(alloc_pages
		    ((kern_bootinfo.kern_end -
		      kern_bootinfo.kern_data) / PGSIZE));

	memmove((char *)VADDR_DIRECT(kern_share),
		kern_data + kern_bootinfo.kern_data - kern_bootinfo.kern_text,
		kern_bootinfo.kern_bss - kern_bootinfo.kern_data);
	memset((char *)VADDR_DIRECT(kern_share) + kern_bootinfo.kern_bss -
	       kern_bootinfo.kern_data, 0,
	       kern_bootinfo.kern_end - kern_bootinfo.kern_bss);
}

void jump_kern(void)
{
	pgd_t *pgdir;
	pgdir = lcpu_static[lapic_id()].init_pgdir =
	    (pgd_t *) page2va(alloc_page());
	memcpy(pgdir, boot_pgdir, PGSIZE);

	pgdir[0] = 0;
	pgdir[PGX(VPT)] = PADDR_DIRECT(pgdir) | PTE_P | PTE_W;

	uintptr_t i;
	for (i = kern_bootinfo.kern_text; i < kern_bootinfo.kern_end;
	     i += PGSIZE) {
		pte_t *pte = get_pte(pgdir, i, 1);
		if (i < kern_bootinfo.kern_pls) {
			/* map all readonly data */
			*pte =
			    PADDR_DIRECT(kern_data + i -
					 kern_bootinfo.kern_text) | PTE_P;
		} else if (i < kern_bootinfo.kern_data) {
			/* alloc all pls data */
			uintptr_t pls_size =
			    kern_bootinfo.kern_data - kern_bootinfo.kern_pls;
			struct Page *pls = alloc_pages(pls_size >> PGSHIFT);
			lcpu_static[lapic_id()].pls_base =
			    (uintptr_t) page2va(pls);
			*pte = page2pa(pls) | PTE_W | PTE_P;
		} else {
			/* Kern share data */
			*pte =
			    (kern_share + i -
			     kern_bootinfo.kern_data) | PTE_W | PTE_P;
		}
	}

	lcr3(PADDR_DIRECT(pgdir));
	/* Setup cr0 protection */
	uint64_t cr0 = rcr0();
	cr0 |=
	    CR0_PE | CR0_PG | CR0_AM | CR0_WP | CR0_NE | CR0_TS | CR0_EM |
	    CR0_MP;
	cr0 &= ~(CR0_TS | CR0_EM);
	lcr0(cr0);

	memmove((char *)kern_bootinfo.kern_pls,
		kern_data + kern_bootinfo.kern_pls - kern_bootinfo.kern_text,
		kern_bootinfo.kern_data - kern_bootinfo.kern_pls);

	((void (*)(void))kern_bootinfo.kern_entry) ();

	while (1) ;
}

void kcons_putc(int c)
{
	cons_putc(c);
}

int kcons_getc(void)
{
	return cons_getc();
}

void intr_handler_set(int intr_no, intr_handler_f h)
{
	if (intr_no >= IRQ_OFFSET && intr_no < IRQ_OFFSET + IRQ_COUNT &&
	    intr_no != IRQ_OFFSET + IRQ_TIMER) {
		ioapic_disable(ioapic_id_set[0], intr_no - IRQ_OFFSET);
		irq_control[intr_no - IRQ_OFFSET].lcpu_apic_id = lapic_id();
	}
	lcpu_static[lapic_id()].intr_handler[intr_no] = h;
}

void irq_enable(int irq_no)
{
	ioapic_enable(ioapic_id_set[0], irq_no,
		      irq_control[irq_no].lcpu_apic_id);
}

void irq_disable(int irq_no)
{
	ioapic_disable(ioapic_id_set[0], irq_no);
}

void irq_ack(int irq_no)
{
	lapic_eoi_send();
	// ioapic_send_eoi(irq_no);
}

pgd_t *init_pgdir_get(void)
{
	return lcpu_static[lapic_id()].init_pgdir;
}

uintptr_t pls_base_get(void)
{
	return lcpu_static[lapic_id()].pls_base;
}

void kpage_private_set(uintptr_t pa, void *data)
{
	pa2page(pa)->private = data;
}

void *kpage_private_get(uintptr_t pa)
{
	return pa2page(pa)->private;
}

uintptr_t kalloc_pages(size_t npages)
{
	return page2pa(alloc_pages(npages));
}

void kfree_pages(uintptr_t pa, size_t npages)
{
	free_pages(pa2page(pa), npages);
}

void tick_init(int freq)
{
	lapic_timer_set(freq);
}

unsigned int lapic_id_get(void)
{
	return lapic_id();
}

unsigned int lcpu_idx_get(void)
{
	return lcpu_id_inv[lapic_id()];
}

unsigned int lcpu_count_get(void)
{
#if HAS_DRIVER_OS
	return sysconf.lcpu_count - 1;
#else
	return sysconf.lcpu_count;
#endif
}

uintptr_t hpet_phys_get(void)
{
	if (sysconf.has_hpet)
		return sysconf.hpet_phys;
	else
		return 0;
}

#if HAS_DRIVER_OS
EXPORT_SYMBOL(driver_os_notify);

volatile void *driver_os_buffer_get(void)
{
	return driver_os_buffer;
}

uint64_t driver_os_buffer_size_get(void)
{
	return driver_os_buffer_size;
}

EXPORT_SYMBOL(driver_os_buffer_get);
EXPORT_SYMBOL(driver_os_buffer_size_get);
#endif

int driver_os_is_enabled(void)
{
#if HAS_DRIVER_OS
	return 1;
#else
	return 0;
#endif
}

EXPORT_SYMBOL(driver_os_is_enabled);

EXPORT_SYMBOL(context_fill);
EXPORT_SYMBOL(context_switch);
EXPORT_SYMBOL(kcons_putc);
EXPORT_SYMBOL(kcons_getc);
EXPORT_SYMBOL(intr_handler_set);
EXPORT_SYMBOL(irq_enable);
EXPORT_SYMBOL(irq_disable);
EXPORT_SYMBOL(irq_ack);
EXPORT_SYMBOL(init_pgdir_get);
EXPORT_SYMBOL(pls_base_get);
EXPORT_SYMBOL(kpage_private_set);
EXPORT_SYMBOL(kpage_private_get);
EXPORT_SYMBOL(kalloc_pages);
EXPORT_SYMBOL(kfree_pages);
EXPORT_SYMBOL(load_rsp0);
EXPORT_SYMBOL(tick_init);
EXPORT_SYMBOL(lapic_id_get);
EXPORT_SYMBOL(lcpu_idx_get);
EXPORT_SYMBOL(lcpu_count_get);
EXPORT_SYMBOL(lapic_ipi_issue);
EXPORT_SYMBOL(hpet_phys_get);
