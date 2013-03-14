.text
.globl switch_to
switch_to:                      # switch_to(from, to)
    # save from's registers
    movq %rdi, %rax             # rax point to from
    popq 0(%rax)                # save rip
    movq %rsp, 0x08(%rax)
    movq %rdi, 0x10(%rax)
    movq %rsi, 0x18(%rax)
    movq %rdx, 0x20(%rax)
    movq %rcx, 0x28(%rax)
    movq %r8,  0x30(%rax)
    movq %r9,  0x38(%rax)
    movq %r10, 0x40(%rax)
    movq %r11, 0x48(%rax)
    movq %rbx, 0x50(%rax)
    movq %rbp, 0x58(%rax)
    movq %r12, 0x60(%rax)
    movq %r13, 0x68(%rax)
    movq %r14, 0x70(%rax)
    movq %r15, 0x78(%rax)

    # restore to's registers
    movq %rsi, %rax

    movq 0x78(%rax), %r15
    movq 0x70(%rax), %r14
    movq 0x68(%rax), %r13
    movq 0x60(%rax), %r12
    movq 0x58(%rax), %rbp
    movq 0x50(%rax), %rbx
    movq 0x48(%rax), %r11
    movq 0x40(%rax), %r10
    movq 0x38(%rax), %r9
    movq 0x30(%rax), %r8
    movq 0x28(%rax), %rcx
    movq 0x20(%rax), %rdx
    movq 0x18(%rax), %rsi
    movq 0x10(%rax), %rdi
    movq 0x08(%rax), %rsp

    pushq 0(%rax)               # push rip

    ret

