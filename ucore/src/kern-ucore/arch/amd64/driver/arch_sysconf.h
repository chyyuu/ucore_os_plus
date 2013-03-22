/**
 * Architecture-dependent fields in sysconf_s
 * Should only be included in sysconf/sysconf.h
 */

int lioapic_count;

//XXX
uintptr_t lapic_phys;

//int ioapic_count;
//int use_ioapic_eoi;

int has_hpet;
uintptr_t hpet_phys;
