#ifndef __LCPU_H__
#define __LCPU_H__

#include <lapic.h>
#include <trap.h>

typedef void (*intr_handler_f) (struct trapframe * tf);

typedef struct lcpu_info_s {
	int lapic_id;
	int idx;
	uint64_t freq;
} lcpu_info_s;

typedef struct lcpu_static_s {
	pgd_t *init_pgdir;
	uintptr_t pls_base;
	intr_handler_f intr_handler[256];

	lcpu_info_s public;
} lcpu_static_s;

typedef struct lcpu_dynamic_s {
} lcpu_dynamic_s;

typedef struct irq_control_s {
	int lcpu_apic_id;
} irq_control_s;

extern irq_control_s irq_control[IRQ_COUNT];

extern unsigned int lcpu_id_set[LAPIC_COUNT];
extern unsigned int lcpu_id_inv[LAPIC_COUNT];
/* Indexed by APIC ID */
extern lcpu_static_s lcpu_static[LAPIC_COUNT];
extern lcpu_dynamic_s lcpu_dynamic[LAPIC_COUNT];

void lcpu_init(void) __attribute__ ((noreturn));

#endif
