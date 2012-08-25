#include <unistd.h>

.text
.globl __clone
__clone:                        # __clone(clone_flags, stack, fn, arg)
    pushl %ebp                  # maintain ebp chain
    movl %esp, %ebp

    pushl %edx                  # save old registers
    pushl %ecx
    pushl %ebx
    pushl %edi

    movl 0x8(%ebp), %edx        # load clone_flags
    movl 0xc(%ebp), %ecx        # load stack
    movl 0x10(%ebp), %ebx       # load fn
    movl 0x14(%ebp), %edi       # load arg

    movl $SYS_clone, %eax       # load SYS_clone
    int $T_SYSCALL              # syscall

    cmpl $0x0, %eax             # pid ? child or parent ?
    je 1f                       # eax == 0, goto 1;

    # parent
    popl %edi                   # restore registers
    popl %ebx
    popl %ecx
    popl %edx

    leave                       # restore ebp
    ret

    # child
1:
    pushl %edi
    call *%ebx                  # call fn(arg)

    movl %eax, %edx             # save exit_code
    movl $SYS_exit_thread, %eax # load SYS_exit_thread
    int $T_SYSCALL              # int SYS_exit_thread

spin:                           # error ?
    jmp spin

