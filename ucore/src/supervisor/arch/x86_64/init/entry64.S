#include <mmu.h>
#include <memlayout.h>

.text
.code64
.global kern_entry64
kern_entry64:
    movq $0x0, %rbp
    movq $SVBASE, %rsp
    addq $bootstacktop - SVBASE, %rsp
    call kern_init

# should never get here
spin:
    jmp spin

.data
.align PGSIZE
    .globl bootstack
bootstack:
    .space KSTACKSIZE
    .globl bootstacktop
bootstacktop:

