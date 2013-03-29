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
    movq $0x0, %rbp
#rsp already setup
    call ap_init
    jmp spin

.data
.align PGSIZE
    .globl bootstack
bootstack:
    .space KSTACKSIZE
    .globl bootstacktop
bootstacktop:

