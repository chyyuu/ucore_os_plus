/* Simple linker script for the ucore kernel.
   See the GNU ld 'info' manual ("info ld") to learn the syntax. */

OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(kern_entry32)

SV_BASE       = 0xFFFF800000000000;
ENTRY32_BASE  =         0x00200000;
BOOT_AP_BASE  =         0x00008000;
BOOT_AP_MAGIC =         0x001F8000;

SECTIONS {

	. = BOOT_AP_BASE;

	PROVIDE(boot_ap_entry_64 = . + BOOT_AP_MAGIC + SV_BASE);

	PROVIDE(__sv_start = . + BOOT_AP_MAGIC);

    .boot_ap : AT(ADDR(.boot_ap) + BOOT_AP_MAGIC) {
        *boot_1ap.S.o*(.text .rodata .data)
    }

	. = ALIGN(0x1000);
	. += BOOT_AP_MAGIC;

    .entry32 : AT(ADDR(.entry32)) {
        *entry32.S.o*(.text .rodata .data)
    }

    . = ALIGN(0x1000);
    . += SV_BASE;

    .text : AT(ADDR(.text) - SV_BASE) {
        *(.text .stub .text.* .gnu.linkonce.t.*)
    }

    PROVIDE(__etext = . - SV_BASE); /* Define the 'etext' symbol to this value */

    .rodata : AT(ADDR(.rodata) - SV_BASE) {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    }

    /* Adjust the address for the data segment to the next page */
    . = ALIGN(0x1000);

    /* The data segment */
    .data : AT(ADDR(.data) - SV_BASE) {
        *(.data)
    }

    PROVIDE(__edata = . - SV_BASE);

    .bss : AT(ADDR(.bss) - SV_BASE) {
        *(.bss)
    }

    PROVIDE(__end = . - SV_BASE);

    /DISCARD/ : {
        *(.eh_frame .note.GNU-stack)
    }
}
