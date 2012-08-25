#include <unistd.h>

.text
.globl __clone
__clone:                        # __clone(clone_flags, stack, fn, arg)
    pushq %rbp                  # maintain rbp chain
    movq %rsp, %rbp

    movq $SYS_clone, %rax       # load SYS_clone
    int $T_SYSCALL              # syscall

    cmpq $0x0, %rax             # pid ? child or parent ?
    je 1f                       # rax == 0, goto 1;

    # parent
    leave                       # restore rbp
    ret

    # child
1:
    movq %rcx, %rdi
    callq *%rdx                 # call fn(arg)

    movq %rax, %rdi             # save exit_code
    movq $SYS_exit_thread, %rax # load SYS_exit_thread
    int $T_SYSCALL              # int SYS_exit_thread

spin:                           # error ?
    jmp spin

