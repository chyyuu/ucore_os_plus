/* Simple linker script for ucore user-level programs.
   See the GNU ld 'info' manual ("info ld") to learn the syntax. */

ENTRY(_start)

SECTIONS {
    /* Load programs at this address: "." means the current address */
    . = 0x800020;

    .text : {
        *(.text .stub .text.* .gnu.linkonce.t.*)
    }

    PROVIDE(etext = .); /* Define the 'etext' symbol to this value */

    .rodata : {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    }

    /* Adjust the address for the data segment to the next page */
    . = ALIGN(0x2000);

    .data : {
        *(.data .data.* .gnu.linkonce.d.*)
    }

    PROVIDE(edata = .);

    .bss : {
        *(.bss)
    }

    PROVIDE(end = .);

    /DISCARD/ : {
        *(.eh_frame .note.GNU-stack .comment .stab*)
    }
}
