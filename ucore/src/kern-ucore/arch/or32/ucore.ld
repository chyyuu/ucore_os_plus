OUTPUT_FORMAT("elf32-or32", "elf32-or32", "elf32-or32")
OUTPUT_ARCH(or32)

SECTIONS {
    /* Load the kernel at this address: "." means the current address */
	. = 0x100;
	.vectors : {
	    *(.vectors)
	}

	. = 0xc0004000;
    .text : AT (ADDR(.text) - 0xc0000000) {
        *(.text .stub .text.* .gnu.linkonce.t.*)
    }

    PROVIDE(etext = .); /* Define the 'etext' symbol to this value */

    .rodata : AT (ADDR(.rodata) - 0xc0000000) {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    }

    /* Adjust the address for the data segment to the next page */
    . = ALIGN(0x2000);

    /* The data segment */
    .data : AT (ADDR(.data) - 0xc0000000) {
        *(.data)
    }

    PROVIDE(edata = .);

    .bss : AT (ADDR(.bss) - 0xc0000000) {
        *(.bss)
    }

    . = ALIGN(0x2000);
    PROVIDE(end = .);

    /DISCARD/ : {
        *(.eh_frame .note.GNU-stack)
    }
}
