#include <memlayout.h>

# vectors.S sends all traps here.
.text
.globl __alltraps
__alltraps:
    # push registers to build a trap frame
    # therefore make the stack look like a struct trapframe
    pushq %rdi
    pushq %rsi
    pushq %rdx
    pushq %rcx
    pushq %rax
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    # push ds, es
    movw %es, %rax
    pushq %rax
    movw %ds, %rax
    pushq %rax

    # use %rdi to pass a pointer to the trapframe as an argument to trap()
    movq %rsp, %rdi

    # call trap(tf), where tf=%rsp
    call trap

    # return falls through to trapret...
.globl __trapret
__trapret:
    # restore registers from stack

    # pop ds, es
    popq %rax
    movq %rax, %ds
    popq %rax
    movq %rax, %es

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rax
    popq %rcx
    popq %rdx
    popq %rsi
    popq %rdi

    # get rid of the trap number and error code
    addq $0x10, %rsp
    iretq

