OUTPUT_FORMAT("elf32-or32", "elf32-or32", "elf32-or32")
OUTPUT_ARCH(or32)

SECTIONS {
    /* Load the kernel at this address: "." means the current address */
	. = 0x1ffe00;

	.text : {
		*(.entry)
	    *(.text)
	}

    /DISCARD/ : {
        *(.eh_frame .note.GNU-stack)
    }
}
