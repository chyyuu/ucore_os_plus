#ifndef __BOOT_AP_S_H__
#define __BOOT_AP_S_H__

/* We copy the mp bootloader into 0x8000 */
#define BOOT_AP_ENTRY  0x8000
#define BOOT_AP_STACK_BASE 0x7FE8
/* Copy the boot CR3 addr here */
#define BOOT_AP_CR3    0x7FF0
/* And copy the lapic addr to below */
#define BOOT_AP_LAPIC_PHYS 0x7FF8

#ifndef __ASSEMBLER__
extern char boot_ap_entry[];
#endif

#endif
