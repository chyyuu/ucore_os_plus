OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(start)
SECTIONS {
    . = 0x7c00;

    .entry : {
		*bootasm.S.o*(.text .rodata .data)	
    }

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
