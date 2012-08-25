/* Thanks to u10proj revision 72 for discarding eh_frame section. */
OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(start)
SECTIONS {
    . = 0x7c00;
	.text : {
	*(.text)
	}
	.data : {
	*(.data)
	}
	.bss : {
	*(.bss)
	}
	/DISCARD/ : {
	*(.eh_*)
	}
}
