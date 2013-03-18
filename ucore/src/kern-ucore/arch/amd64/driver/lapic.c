#include <lapic.h>
#include <pmm.h>
#include <sysconf.h>
#include <trap.h>
#include <arch.h>

/* The LAPIC access */
// Local APIC registers, divided by 4 for use as uint32[] indices.
#define ID      (0x0020/4)	// ID
#define VER     (0x0030/4)	// Version
#define TPR     (0x0080/4)	// Task Priority
#define EOI     (0x00B0/4)	// EOI
#define SVR     (0x00F0/4)	// Spurious Interrupt Vector
#define ENABLE     0x00000100	// Unit Enable
#define ESR     (0x0280/4)	// Error Status
#define CMCI    (0x02F0/4)	// CMCI LVT
#define ICRLO   (0x0300/4)	// Interrupt Command
#define INIT       0x00000500	// INIT/RESET
#define STARTUP    0x00000600	// Startup IPI
#define DELIVS     0x00001000	// Delivery status
#define ASSERT     0x00004000	// Assert interrupt (vs deassert)
#define LEVEL      0x00008000	// Level triggered
#define BCAST      0x00080000	// Send to all APICs, including self.
#define ICRHI   (0x0310/4)	// Interrupt Command [63:32]
#define TIMER   (0x0320/4)	// Local Vector Table 0 (TIMER)
#define X1         0x0000000B	// divide counts by 1
#define PERIODIC   0x00020000	// Periodic
#define TMINT   (0x0330/4)	// Thermal Monitor
#define PCINT   (0x0340/4)	// Performance Counter LVT
#define LINT0   (0x0350/4)	// Local Vector Table 1 (LINT0)
#define LINT1   (0x0360/4)	// Local Vector Table 2 (LINT1)
#define ERROR   (0x0370/4)	// Local Vector Table 3 (ERROR)
#define MASKED     0x00010000	// Interrupt masked
#define TICR    (0x0380/4)	// Timer Initial Count
#define TCCR    (0x0390/4)	// Timer Current Count
#define TDCR    (0x03E0/4)	// Timer Divide Configuration

#define LAPIC_PERIODIC 10000000

static uint32_t lapicr(int index)
{
	return ((volatile uint32_t *)VADDR_DIRECT(sysconf.lapic_phys))[index];
}

static uint32_t lapicw(int index, uint32_t value)
{
	((volatile uint32_t *)VADDR_DIRECT(sysconf.lapic_phys))[index] = value;
	return ((volatile uint32_t *)VADDR_DIRECT(sysconf.lapic_phys))[ID];
}

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

int lapic_id(void)
{
	return (lapicr(ID) >> 24) & 0xFF;
}

int lapic_init(void)
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

// Acknowledge interrupt.
void lapic_eoi_send(void)
{
	lapicw(EOI, 0);
}

// Spin for a given number of microseconds.
// On real hardware would want to tune this dynamically.
static void microdelay(int us)
{
	volatile int j = 0;

	while (us-- > 0)
		for (j = 0; j < 10000; j++) ;
}

/* CODE FROM XV6 {{{ */
#define IO_RTC  0x70

// Start additional processor running bootstrap code at addr.
// See Appendix B of MultiProcessor Specification.
void lapic_ap_start(int apicid, uint32_t addr)
{
	int i;
	uint16_t *wrv;

	// "The BSP must initialize CMOS shutdown code to 0AH
	// and the warm reset vector (DWORD based at 40:67) to point at
	// the AP startup code prior to the [universal startup algorithm]."
	outb(IO_RTC, 0xF);	// offset 0xF is shutdown code
	outb(IO_RTC + 1, 0x0A);
	wrv = VADDR_DIRECT(0x40 << 4 | 0x67);
	wrv[0] = addr & 0xffff;
	wrv[1] = (addr ^ wrv[0]) >> 4;

	// "Universal startup algorithm."
	// Send INIT (level-triggered) interrupt to reset other CPU.
	lapicw(ICRHI, apicid << 24);
	lapicw(ICRLO, INIT | LEVEL | ASSERT);
	microdelay(200);
	lapicw(ICRLO, INIT | LEVEL);
	microdelay(100);	// should be 10ms, but too slow in Bochs!

	// Send startup IPI (twice!) to enter bootstrap code.
	// Regular hardware is supposed to only accept a STARTUP
	// when it is in the halted state due to an INIT.  So the second
	// should be ignored, but it is part of the official Intel algorithm.
	// Bochs complains about the second one. Too bad for Bochs.
	for (i = 0; i < 2; i++) {
		lapicw(ICRHI, apicid << 24);
		lapicw(ICRLO, STARTUP | (addr >> 12));
		microdelay(200);
	}
}

/* }}} */

void lapic_timer_set(uint32_t freq)
{
	if (freq == 0) {
		// Disable timer
		lapicw(TIMER, MASKED);
	} else {
		// The timer repeatedly counts down at bus frequency
		// from lapic[TICR] and then issues an interrupt.
		// If xv6 cared more about precise timekeeping,
		// TICR would be calibrated using an external time source.

		lapicw(TDCR, X1);
		lapicw(TIMER, PERIODIC | (IRQ_OFFSET + IRQ_TIMER));
		lapicw(TICR, LAPIC_PERIODIC);
	}
}

/* XXX */
int lapic_ipi_issue(int lapic_id)
{
	return lapic_ipi_issue_spec(lapic_id, T_IPI);
}

int lapic_ipi_issue_spec(int lapic_id, uint8_t vector)
{
	if (lapicr(ICRLO) & DELIVS)
		return -1;

	lapicw(ICRHI, lapic_id << 24);
	lapicw(ICRLO, ASSERT | vector);

	while (lapicr(ICRLO) & DELIVS) ;
	return 0;
}
