#include <system.h>
#include <memlayout.h>
#include <nios2.h>
.set nobreak
.set noat
        .section .exceptions.entry.label, "xa"

        .globl alt_exception
        .type alt_exception, @function
alt_exception:

        .section .exceptions.entry, "xa"

#r24=et
#r30=ba, saves kernel sp for processes.
    rdctl et, estatus
    andi  et, et, NIOS2_STATUS_U_MSK
    beq   et, r0, __trap_in_kernel
    
#This trap happen in user mode. We have to change to kernel stack.
    mov   et, sp
    mov   sp, r30
#save user sp.
    addi  sp, sp, -4
    stw   et, 0(sp)
    
.global __trap_in_kernel
__trap_in_kernel:

    addi  sp, sp, -108
    
    stw   ra,  0(sp)
    
    stw   r1,   8(sp)
    stw   r2,  12(sp)
    stw   r3,  16(sp)
    stw   r4,  20(sp)
    stw   r5,  24(sp)
    stw   r6,  28(sp)
    stw   r7,  32(sp)

    rdctl r5, estatus

    stw   r8,  36(sp)
    stw   r9,  40(sp)
    stw   r10, 44(sp)
    stw   r11, 48(sp)
    stw   r12, 52(sp)
    stw   r13, 56(sp)
    stw   r14, 60(sp)
    stw   r15, 64(sp)
    stw   r16, 68(sp)
    stw   r17, 72(sp)
    stw   r18, 76(sp)
    stw   r19, 80(sp)
    stw   r20, 84(sp)
    stw   r21, 88(sp)
    stw   r22, 92(sp)
    stw   r23, 96(sp)
    
    /*
     * ea-4 contains the address of the instruction being executed
     * when the exception occured. For interrupt exceptions, we will
     * will be re-issue the isntruction. Store it in 4(sp)
     */
    stw   r5,  100(sp)  /* estatus */
    stw   fp,  104(sp)

    addi  r15, ea, -4  /* instruction that caused exception */
    stw   r15,  4(sp)
    
    mov   r30, sp
    
#r4 is the 1st param in a function. Now it becomes trapframe*
    mov   r4, sp

    call trap
    
.globl __trapret
__trapret:
    
    ldw   ea,  4(sp)  /* This becomes the PC once eret is executed */
    ldw   ra,   0(sp)

    ldw   r1,   8(sp)
    ldw   r2,  12(sp)
    ldw   r3,  16(sp)
    ldw   r4,  20(sp)
    ldw   r5,  24(sp)
    ldw   r6,  28(sp)
    ldw   r7,  32(sp)

    ldw   r8,  36(sp)
    ldw   r9,  40(sp)
    ldw   r10, 44(sp)
    ldw   r11, 48(sp)
    ldw   r12, 52(sp)
    ldw   r13, 56(sp)
    ldw   r14, 60(sp)
    ldw   r15, 64(sp)
    ldw   r16, 68(sp)
    ldw   r17, 72(sp)
    ldw   r18, 76(sp)
    ldw   r19, 80(sp)
    ldw   r20, 84(sp)
    ldw   r21, 88(sp)
    ldw   r22, 92(sp)
    ldw   r23, 96(sp)
    
    ldw   et,  100(sp)
    wrctl estatus, et
    ldw   fp,  104(sp)
    
    addi  sp, sp, 108

    andi  et, et, NIOS2_STATUS_U_MSK
    beq   et, r0, __trap_end

#This trap happen in user mode. We have to restore user stack.
    addi  r30, sp, 4
    ldw   sp,   0(sp)

.global __trap_end
__trap_end:

    eret

.globl forkrets
forkrets:
    # set stack to this new process's trapframe
    mov sp, r4
    br __trapret
    
