#include <lapic.h>
#include <pmm.h>
#include <sysconf.h>
#include <trap.h>
#include <arch.h>
#include <cpuid.h>
#include <kio.h>
#include <picirq.h>
#include <percpu.h>
#include <sync.h>

/* The LAPIC access */
// Local APIC registers, divided by 4 for use as uint[] indices.
#define ID      (0x0020/4)   // ID
#define VER     (0x0030/4)   // Version
#define TPR     (0x0080/4)   // Task Priority
#define PPR     (0x00A0/4)   // Processor Priority
#define EOI     (0x00B0/4)   // EOI
#define LDR     (0x00D0/4)   // Logical Destination
#define SVR     (0x00F0/4)   // Spurious Interrupt Vector
  #define ENABLE     0x00000100   // Unit Enable
#define ISR     (0x0100/4)   // In-service register
  #define ISR_NR     0x8
#define TMR     (0x0180/4)   // Trigger mode register
#define IRR     (0x0200/4)   // Interrupt request register
#define ESR     (0x0280/4)   // Error Status
#define CMCI    (0x02f0/4)   // CMCI LVT
#define ICRLO   (0x0300/4)   // Interrupt Command
  #define INIT       0x00000500   // INIT/RESET
  #define STARTUP    0x00000600   // Startup IPI
  #define DELIVS     0x00001000   // Delivery status
  #define ASSERT     0x00004000   // Assert interrupt (vs deassert)
  #define DEASSERT   0x00000000
  #define LEVEL      0x00008000   // Level triggered
  #define BCAST      0x00080000   // Send to all APICs, including self.
  #define FIXED      0x00000000
#define ICRHI   (0x0310/4)   // Interrupt Command [63:32]
#define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
  #define X1         0x0000000B   // divide counts by 1
  #define PERIODIC   0x00020000   // Periodic
#define THERM   (0x0330/4)   // Thermal sensor LVT
#define TMINT   (0x0330/4)	// Thermal Monitor
#define PCINT   (0x0340/4)   // Performance Counter LVT
#define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
#define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
#define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
  #define MASKED     0x00010000   // Interrupt masked
  #define MT_NMI     0x00000400   // NMI message type
  #define MT_FIX     0x00000000   // Fixed message type
#define TICR    (0x0380/4)   // Timer Initial Count
#define TCCR    (0x0390/4)   // Timer Current Count
#define TDCR    (0x03E0/4)   // Timer Divide Configuration

#define IO_RTC  0x70



#define LAPIC_PERIODIC 10000000

//XXX
#define QUANTUM 100

static volatile uint32_t *xapic;
static uint64_t xapichz;

static void
xapicw(uint32_t index, uint32_t value)
{
  xapic[index] = value;
  xapic[ID];  // wait for write to finish, by reading
}

static uint32_t
xapicr(uint32_t off)
{
  return xapic[off];
}


static inline void mask_pc(int mask)
{
	xapicw(PCINT, mask ? MASKED : MT_NMI);
}


static int
xapicwait()
{
  int i = 100000;
  while ((xapicr(ICRLO) & DELIVS) != 0) {
    nop_pause();
    i--;
    if (i == 0) {
      kprintf("xapicwait: wedged?\n");
      return -1;
    }
  }
  return 0;
}



static uint32_t lapicr(int index)
{
	return ((volatile uint32_t *)VADDR_DIRECT(sysconf.lapic_phys))[index];
}

static uint32_t lapicw(int index, uint32_t value)
{
	((volatile uint32_t *)VADDR_DIRECT(sysconf.lapic_phys))[index] = value;
	return ((volatile uint32_t *)VADDR_DIRECT(sysconf.lapic_phys))[ID];
}

#if 0
int lapic_init_ap(void)
{
	// Enable local APIC; set spurious interrupt vector.
	lapicw(SVR, ENABLE | (IRQ_OFFSET + IRQ_SPURIOUS));

	// Disable timer
	lapicw(TIMER, MASKED);

	// Disable logical interrupt lines.
	lapicw(LINT0, MASKED);
	lapicw(LINT1, MASKED);

	/* Disable the TM interrupt */
	if (((lapicr(VER) >> 16) & 0xFF) >= 5)
		lapicw(TMINT, MASKED);

	// Disable performance counter overflow interrupts
	// on machines that provide that interrupt entry.
	if (((lapicr(VER) >> 16) & 0xFF) >= 4)
		lapicw(PCINT, MASKED);

	// Map error interrupt to IRQ_ERROR.
	lapicw(ERROR, IRQ_OFFSET + IRQ_ERROR);

	// Clear error status register (requires back-to-back writes).
	lapicw(ESR, 0);
	lapicw(ESR, 0);

	// Ack any outstanding interrupts.
	lapicw(EOI, 0);

	// Send an Init Level De-Assert to synchronise arbitration ID's.
	lapicw(ICRHI, 0);
	lapicw(ICRLO, BCAST | INIT | LEVEL);
	while (lapicr(ICRLO) & DELIVS) ;

	// Enable interrupts on the APIC (but not on the processor).
	lapicw(TPR, 0);

	// Bochs doesn't support IMCR, so this doesn't run on Bochs.
	// But it would on real hardware.
	/* outb(0x22, 0x70);   // Select IMCR */
	/* outb(0x23, inb(0x23) | 1);  // Mask external interrupts. */

	return 0;
}
#endif

static uint32_t x_lapic_id(struct lapic_chip* chip)
{
	if (read_rflags() & FL_IF) {
		cli();
		panic("xapic_lapic::id() called from %p with interrupts enabled\n",
				__builtin_return_address(0));
	}

	return (xapic[ID] >> 24) & 0xFF;
}

// Acknowledge interrupt.
static
void lapic_eoi_send(struct lapic_chip* chip)
{
	xapicw(EOI, 0);
}

static void x_lapic_start_ap(struct lapic_chip *_this, struct cpu *c,
			     uint32_t addr)
{
	int i;

	// "Universal startup algorithm."
	// Send INIT (level-triggered) interrupt to reset other CPU.

	xapicw(ICRHI, c->hwid << 24);
	xapicw(ICRLO, INIT | LEVEL | ASSERT);
	xapicwait();
	microdelay(10000);
	xapicw(ICRLO, INIT | LEVEL);
	xapicwait();
	microdelay(10000);    // should be 10ms, but too slow in Bochs!

	// Send startup IPI (twice!) to enter bootstrap code.
	// Regular hardware is supposed to only accept a STARTUP
	// when it is in the halted state due to an INIT.  So the second
	// should be ignored, but it is part of the official Intel algorithm.
	// Bochs complains about the second one.  Too bad for Bochs.
	for(i = 0; i < 2; i++){
		xapicw(ICRHI, c->hwid << 24);
		xapicw(ICRLO, STARTUP | (addr>>12));
		microdelay(200);
	}
}


static void x_cpu_init(struct lapic_chip* _this)
{
	uint64_t count;

	kprintf("xapic: Initializing LAPIC (CPU %d)\n", myid());

	// Enable local APIC, do not suppress EOI broadcast, set spurious
	// interrupt vector.
	xapicw(SVR, ENABLE | (IRQ_OFFSET + IRQ_SPURIOUS));

	if (xapichz == 0) {
		// Measure the TICR frequency
		xapicw(TDCR, X1);
		xapicw(TICR, 0xffffffff); 
		uint64_t ccr0 = xapicr(TCCR);
		microdelay(10 * 1000);    // 1/100th of a second
		uint64_t ccr1 = xapicr(TCCR);
		xapichz = 100 * (ccr0 - ccr1);
	}

	count = (QUANTUM*xapichz) / 1000;
	if (count > 0xffffffff)
		panic("initxapic: QUANTUM too large");

	// The timer repeatedly counts down at bus frequency
	// from xapic[TICR] and then issues an interrupt.  
	xapicw(TDCR, X1);
	xapicw(TIMER, PERIODIC | (IRQ_OFFSET + IRQ_TIMER));
	xapicw(TICR, count); 

	// Disable logical interrupt lines.
	xapicw(LINT0, MASKED);
	xapicw(LINT1, MASKED);

	// Disable performance counter overflow interrupts
	// on machines that provide that interrupt entry.
	if(((xapic[VER]>>16) & 0xFF) >= 4)
		mask_pc(0);

	// Map error interrupt to IRQ_ERROR.
	xapicw(ERROR, IRQ_OFFSET + IRQ_ERROR);

	// Clear error status register (requires back-to-back writes).
	xapicw(ESR, 0);
	xapicw(ESR, 0);

	// Ack any outstanding interrupts.
	xapicw(EOI, 0);

	// Send an Init Level De-Assert to synchronise arbitration ID's.
	xapicw(ICRHI, 0);
	xapicw(ICRLO, BCAST | INIT | LEVEL);
	while(xapic[ICRLO] & DELIVS)
		;

	// Enable interrupts on the APIC (but not on the processor).
	xapicw(TPR, 0);

	return;
}
static void x_init_late(struct lapic_chip* c)
{
	
	uint64_t apic_base = readmsr(MSR_APIC_BAR);
	/* map xapic registers */
	*get_pte(boot_pgdir, (uintptr_t)xapic, 1) = apic_base | PTE_P | PTE_W;
}
	
static void x_lapic_send_ipi(struct lapic_chip* thiz, struct cpu* c, int ino)
{
	bool intr_flag;
	local_intr_save(intr_flag);
	xapicw(ICRHI, c->hwid << 24);
	xapicw(ICRLO, FIXED | DEASSERT | ino);
	if (xapicwait() < 0)
		panic("xapic_lapic::send_ipi: xapicwait failure");

	local_intr_restore(intr_flag);
}

static struct lapic_chip xapic_chip = {
	.cpu_init = x_cpu_init,
	.id = x_lapic_id,
	.eoi = lapic_eoi_send,
	.init_late = x_init_late,
	.start_ap = x_lapic_start_ap,
	.send_ipi = x_lapic_send_ipi,
};

static xapic_init_once()
{
	uint64_t apic_base = readmsr(MSR_APIC_BAR);

	// Sanity check
	if (!(apic_base & APIC_BAR_XAPIC_EN))
		panic("xapic_lapic::init xAPIC not enabled");

	xapic = (uint32_t*)VADDR_DIRECT(apic_base & ~0xffful);
	//map xapic registers
	//*get_pte(boot_pgdir, (uintptr_t)xapic, 1) =
	//	apic_base | PTE_P | PTE_W;

	kprintf("xapic_init_once: xapic base %p\n", xapic);

}

struct lapic_chip* xapic_lapic_init_early(void)
{
	if(!cpuid_check_feature(CPUID_FEATURE_APIC))
		return NULL;
	/* init lapic_chip */
	xapic_init_once();
	kprintf("xapic_lapic_init: Using xapic LAPIC\n");	
	return &xapic_chip;
}


