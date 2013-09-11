#include <mmu.h>
#include <memlayout.h>

.text
.code64
.global kern_entry64
kern_entry64:
    movq $0x0, %rbp
    movq $KERNBASE, %rsp
    addq $bootstacktop - KERNBASE, %rsp
    call kern_init

# should never get here
spin:
    jmp spin

.global kern_ap_entry64
kern_ap_entry64:
    # setup rsp
    mov (%esp), %rsp

    # boot_cr3 cr3
    mov  (%ebp), %rax
    mov  %rax, %cr3

    movq $0x0, %rbp

    call ap_init
    jmp spin

.data
.align PGSIZE
    .globl bootstack
bootstack:
    .space KSTACKSIZE
    .globl bootstacktop
bootstacktop:

