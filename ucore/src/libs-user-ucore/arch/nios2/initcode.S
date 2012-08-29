.text
.globl _start
_start:
    # set ebp for backtrace
    # movl $0x0, %ebp

    # move down the esp register
    # since it may cause page fault in backtrace
    # subl $0x20, %esp
    addi  sp, sp, -0x20
    
    #movhi gp, %hi(_gp)
    #ori gp, gp, %lo(_gp)


    # call user-program function
    call umain
initcode_end:
    br initcode_end

