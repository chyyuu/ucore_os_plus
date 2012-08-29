#include <unistd.h>

        .text
        .global __clone
__clone:                              # __clone(clone_flags, stack, fn, arg)
#        addi sp, sp, -0x4
#        stw r7, 0(sp)                 # store arg
        mov r8, r7
        mov r7, r6
        mov r6, r5
        mov r5, r4
        movhi r4, %hi(SYS_clone)
        ori r4, r4, %lo(SYS_clone)
        trap
        beq r2, r0, son
parent:                               #parent
#        addi sp, sp, 0x4
        ret        
son:
#        ldw r4, 0(sp)                 #now r4 becomes arg
#        addi sp, sp, 0x4
        mov r4, r8
        callr r7                      #r7 stores fn
        mov r5, r2                    #r2 is the exit code
        movhi r4, %hi(SYS_exit_thread)
        ori r4, r4, %lo(SYS_exit_thread)
        trap
        
clone_end:
    br clone_end

