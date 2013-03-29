.text
.globl _start
_start:
    # set rbp for backtrace
    movq $0x0, %rbp

    # move down the rsp register
    # since it may cause page fault in backtrace
    subq $0x20, %rsp

    # call user-program function
    call umain
1:  jmp 1b

