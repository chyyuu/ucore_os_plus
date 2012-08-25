	.text
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
		
.globl forkrets
forkrets:
    # set stack to this new process's trapframe
    movq %rdi, %rsp
    jmp __trapret
