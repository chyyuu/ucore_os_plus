#include <lcpu.h>
#include <sysconf.h>
#include <ioapic.h>
#include <lapic.h>
#include <trap.h>
#include <clock.h>
#include <kern.h>
#include <atom.h>
#include <lapic.h>
#include <string.h>
#include <boot_ap.S.h>
#include <stdio.h>
#include <arch.h>
#include <mmu.h>

irq_control_s irq_control[IRQ_COUNT];
unsigned int lcpu_id_set[LAPIC_COUNT];
unsigned int lcpu_id_inv[LAPIC_COUNT];
lcpu_static_s lcpu_static[LAPIC_COUNT];
lcpu_dynamic_s lcpu_dynamic[LAPIC_COUNT];

volatile int lcpu_init_count = 0;

void lcpu_init(void)
{
	while (1) {
		int old = lcpu_init_count;
		if (cmpxchg32(&lcpu_init_count, old, old + 1) == old)
			break;
	}

	jump_kern();
}
