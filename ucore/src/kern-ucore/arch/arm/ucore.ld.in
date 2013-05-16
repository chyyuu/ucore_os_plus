/* Simple linker script for the ucore kernel.
   See the GNU ld 'info' manual ("info ld") to learn the syntax. */

OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(kern_entry)

SECTIONS {
    /* Load the kernel at this address: "." means the current address */
    . = 0xc0008000;
    PROVIDE(__kernel_text_start = .);

    .text : {
	  *(.entrytext)
      *(.text .stub .text.* .gnu.linkonce.t.*)
      *(.init.text)
      . = ALIGN(4);
        PROVIDE(__initcall_start = .);
        *(.initcall1.init)
        *(.initcall2.init)
        *(.initcall3.init)
        *(.initcall4.init)
        *(.initcall5.init)
        *(.initcall6.init)
        *(.initcall7.init)
        PROVIDE(__initcall_end = .);
    }

    . = ALIGN(4096);
    PROVIDE(__kernel_text_end = .);
    PROVIDE(etext = .); /* Define the 'etext' symbol to this value */

    .rodata : {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
        . = ALIGN(4);
        PROVIDE(__arch_info_begin = .);
        *(.arch.info.init)
        PROVIDE(__arch_info_end = .);
        . = ALIGN(4);
        PROVIDE(__start___param = .);
        *(__param)
        PROVIDE(__stop___param = .);
        . = ALIGN(4);
        PROVIDE(__start___ksymtab = .);
        *(__ksymtab)
        PROVIDE(__stop___ksymtab = .);
        . = ALIGN(4);
        PROVIDE(__start___ksymtab_gpl = .);
        *(__ksymtab_gpl)
        PROVIDE(__stop___ksymtab_gpl = .);
        . = ALIGN(4);
        *(__ksymtab_strings)
    }

    /* Include debugging information in kernel memory */
    .stab : {
        PROVIDE(__STAB_BEGIN__ = .);
        *(.stab);
        PROVIDE(__STAB_END__ = .);
    }

    .stabstr : {
        PROVIDE(__STABSTR_BEGIN__ = .);
        *(.stabstr);
        PROVIDE(__STABSTR_END__ = .);
    }

    /* Adjust the address for the data segment to the next page */
    . = ALIGN(0x1000);

    /* The data segment */
    /* We don't link .initrd into another section because
     * QEMU only load .text and .data
     * However, our own loader will load all sections.
     */
    .data : {
        *(.data)
        *(.init.data)
        . = ALIGN(4096);
    PROVIDE(initrd_begin = .);
      *(.initrd)
      . = ALIGN(4096); 
    PROVIDE(initrd_end = .);
    }

    PROVIDE(edata = .);

    .bss : {
        *(.bss)
    }
    . = ALIGN(0x1000);

    .percpu : {
        PROVIDE(__percpu_start = .);
        *(.percpu)
        PROVIDE(__percpu_end = .);
    }

    PROVIDE(end = .);

    /DISCARD/ : {
        *(.eh_frame .note.GNU-stack)
    }
}
