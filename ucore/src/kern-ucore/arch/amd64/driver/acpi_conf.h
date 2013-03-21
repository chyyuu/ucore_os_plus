#ifndef __ACPI_CONF_H__
#define __ACPI_CONF_H__

/* ACPI module read the system configuration to guide the after
 * initialization. */

//TODO deprecated
int acpi_conf_init(void);

int acpitables_init(void);
int lapic_init(void); 
int numa_init(void);
int cpus_init(void);

int acpi_init(void);
#endif
