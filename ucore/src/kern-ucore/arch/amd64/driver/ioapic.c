#include <ioapic.h>
#include <sysconf.h>
#include <pmm.h>
#include <trap.h>
#include <arch.h>
#include <stdio.h>

/* I/O APIC Code from xv6 */
#define REG_ID     0x00  // Register index: ID
#define REG_VER    0x01  // Register index: version
#define REG_TABLE  0x10  // Redirection table base

// The redirection table starts at REG_TABLE and uses
// two registers to configure each interrupt.  
// The first (low) register in a pair contains configuration bits.
// The second (high) register contains a bitmask telling which
// CPUs can serve that interrupt.
#define INT_DISABLED   0x00010000  // Interrupt disabled
#define INT_LEVEL      0x00008000  // Level-triggered (vs edge-)
#define INT_IRR        0x00004000  // Remote IRR (RO)
#define INT_ACTIVELOW  0x00002000  // Active low (vs high)
#define INT_DELIVS     0x00001000  // Delivery status (RO)
#define INT_LOGICAL    0x00000800  // Destination is CPU id (vs APIC ID)
#define INT_REMAPPABLE (1ull<<48)  // IOMMU remappable


// IO APIC MMIO structure: write reg, then read or write data.
struct ioapic_mmio_s {
	uint32_t reg;
	uint32_t __pad0[3];
	uint32_t data;
	uint32_t __pad1[3];
	uint32_t irq_pin_ass;
	uint32_t __pad2[7];
	uint32_t eoi;
};

typedef volatile struct ioapic_mmio_s ioapic_mmio_s;

static uint32_t ioapic_read(ioapic_mmio_s * ioapic, uint32_t reg)
{
	ioapic->reg = reg;
	return ioapic->data;
}

static void ioapic_write(ioapic_mmio_s * ioapic, uint32_t reg, uint32_t data)
{
	ioapic->reg = reg;
	ioapic->data = data;
}

int __ioapic_hwid_to_id[256];
ioapic_s ioapic[MAX_IOAPIC];

extern int acpi_ioapic_setup(void);
int ioapic_init(void)
{
	acpi_ioapic_setup();

	if(!sysconf.lioapic_count){
		kprintf("ioapic_init: no IOAPICs found, use PIC\n");
		return -1;
	}
	/* maybe not needed
	 * see MP spec3.6 
	 */	
	outb(0x22, 0x70);   // Select IMCR
	outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
	return 0;
}

// XXX THIS ROUTINE IS BUGGY
void ioapic_send_eoi(int id, int irq)
{
	ioapic_mmio_s *ioapic_mmio =
	    (ioapic_mmio_s *) VADDR_DIRECT(ioapic[id].phys);
	ioapic_mmio->eoi = irq & 0xFF;
}

void ioapic_enable(int id, int irq, int cpunum)
{
	assert(id < sysconf.lioapic_count);
	ioapic_mmio_s *ioapic_mmio =
	    (ioapic_mmio_s *) VADDR_DIRECT(ioapic[id].phys);
	// Mark interrupt edge-triggered, active high,
	// enabled, and routed to the given cpunum,
	// which happens to be that cpu's APIC ID.
	ioapic_write(ioapic_mmio, REG_TABLE + 2 * irq, IRQ_OFFSET + irq);
	ioapic_write(ioapic_mmio, REG_TABLE + 2 * irq + 1, cpunum << 24);
}

void ioapic_disable(int id, int irq)
{
	assert(id < sysconf.lioapic_count);
	ioapic_mmio_s *ioapic_mmio =
	    (ioapic_mmio_s *) VADDR_DIRECT(ioapic[id].phys);
	// Mark interrupt edge-triggered, active high,
	// enabled, and routed to the given cpunum,
	// which happens to be that cpu's APIC ID.
	ioapic_write(ioapic_mmio, REG_TABLE + 2 * irq,
		     INT_DISABLED | (IRQ_OFFSET + irq));
	ioapic_write(ioapic_mmio, REG_TABLE + 2 * irq + 1, 0);
}

void ioapic_register_one(ioapic_s *one)
{
	int i;
	ioapic_mmio_s *ioapic_mmio =
	    (ioapic_mmio_s *) VADDR_DIRECT(one->phys);
	//map it
	*get_pte(boot_pgdir, (uintptr_t)ioapic_mmio, 1) =
		one->phys | PTE_P | PTE_W;
	uint32_t verreg = ioapic_read(ioapic_mmio, REG_VER);
	int ver = verreg & 0xFF;
	int maxintr = (verreg>>16) & 0xFF;
  	assert(!(ver < 0x10 || maxintr > 239));
	one->limit = one->intr_base + maxintr - 1;
	kprintf("ioapic: IOAPIC version: %d, for IRQs: %d..%d at %p(VA)\n", ver, one->intr_base, one->limit, ioapic_mmio);
	for(i = 0; i <= maxintr; i++) {
		ioapic_write(ioapic_mmio, REG_TABLE+2*i, INT_DISABLED | (IRQ_OFFSET + i));
		ioapic_write(ioapic_mmio, REG_TABLE+2*i+1, 0);
  	}


}

