#include <boot_ap.S.h>
#include <mmu.h>
#include <memlayout.h>

		.text
		.code16
		.global boot_ap_entry
boot_ap_entry:

	cli                                             # Disable interrupts
    cld                                             # String operations increment

	# Set up the important data segment registers (DS, ES, SS).
    xorw %ax, %ax                                   # Segment number zero
    movw %ax, %ds                                   # -> Data Segment
    movw %ax, %es                                   # -> Extra Segment
    movw %ax, %ss                                   # -> Stack Segment

    # Enable A20:
    #  For backwards compatibility with the earliest PCs, physical
    #  address line 20 is tied low, so that addresses higher than
    #  1MB wrap around to zero by default. This code undoes this.
seta20.1:
    inb $0x64, %al                                  # Wait for not busy
    testb $0x2, %al
    jnz seta20.1

    movb $0xd1, %al                                 # 0xd1 -> port 0x64
    outb %al, $0x64

seta20.2:
    inb $0x64, %al                                  # Wait for not busy
    testb $0x2, %al
    jnz seta20.2
    movb $0xdf, %al                                 # 0xdf -> port 0x60
    outb %al, $0x60

	// Out 0xFF to 0xA1 and 0x21 to disable all IRQs.
	movb $0xFF, %al                      
	outb %al, $0xa1
	outb %al, $0x21

	cld
	cli

	movl $idt, %eax
	lidt (%eax)

	movl $0xA0, %eax
	movl %eax, %cr4

	movl BOOT_AP_CR3, %eax
	movl %eax, %cr3

	movl $0xC0000080, %ecx
	rdmsr
	orl  $0x00000100, %eax
	wrmsr
		
	movl %cr0, %eax
	orl  $0x80000001, %eax
	movl %eax, %cr0

	movl $boot_ap_gdtdesc, %eax
	lgdt (%eax)
		
	ljmp $KERNEL_CS, $longmode_entry
	
		.code64
longmode_entry:
		mov $KERNEL_DS, %ax
		mov %ax, %ds
		mov %ax, %es
		mov %ax, %ss

		// GET THE APIC ID
		movq BOOT_AP_LAPIC_PHYS, %rbx
		movq $PHYSBASE, %rax
		addq %rax, %rbx
		movl 0x20(%rbx), %eax
		// Tricky to get the stack address
		shrq $12, %rax
		movq BOOT_AP_STACK_BASE, %rbx
		addq %rbx, %rax

		movq $0, %rbp
		movq %rax, %rsp

		movq $SVBASE, %rax
		addq $(ap_boot_init - SVBASE), %rax
		call *%rax

ap_spin:
		jmp ap_spin

.align 4
		
idt:
		.word 0
		.quad 0

boot_ap_gdt:
    SEG_NULL()
    SEG_CODE(STA_X | STA_R)
    SEG_DATA(STA_W)

boot_ap_gdtdesc:
    .word 0x2f
    .quad boot_ap_gdt

 
		.align 4096
		.global __boot_dos_entry
		.code16
__boot_dos_entry:
		movw $0x1000, %ax
		movw %ax, %ds
		movw %ax, %es
		movw %ax, %ss
		movw %ax, %fs
		movw %ax, %gs
		movw $0xe000, %ax
		movw %ax, %sp
		ljmp $0x1020, $0x0000
		
