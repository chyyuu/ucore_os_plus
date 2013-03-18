#ifndef __SYSCONF_H__
#define __SYSCONF_H__

#include <types.h>

typedef struct sysconf_s {
	int lcpu_boot;
	int lcpu_count;

	uintptr_t lapic_phys;

	int ioapic_count;
	int use_ioapic_eoi;

	int has_hpet;
	uintptr_t hpet_phys;
} sysconf_s;

extern sysconf_s sysconf;

#endif
