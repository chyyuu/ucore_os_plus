.set noat
.set nobreak
.text
.globl switch_to
switch_to:                      # switch_to(from, to)

    # save from's registers
    stw   sp,  0(r4)
    stw   r1,  4(r4)
    stw   r2,  8(r4)
    stw   r3,  12(r4)
    stw   r6,  16(r4)
    stw   r7,  20(r4)
    stw   r8,  24(r4)
    stw   r9,  28(r4)
    stw   r10,  32(r4)
    stw   r11,  36(r4)
    stw   r12,  40(r4)
    stw   r13,  44(r4)
    stw   r14,  48(r4)
    stw   r15,  52(r4)
    stw   r16,  56(r4)
    stw   r17,  60(r4)
    stw   r18,  64(r4)
    stw   r19,  68(r4)
    stw   r20,  72(r4)
    stw   r21,  76(r4)
    stw   r22,  80(r4)
    stw   r23,  84(r4)
    stw   fp,  88(r4)
    stw   ea,  92(r4)
    stw   ra,  96(r4)
    stw   gp,  100(r4)
    stw   r30, 108(r4)
    
    rdctl r1,  status
    stw   r1,  104(r4)

    # restore to's registers
    ldw   r1,  104(r5)
    wrctl status, r1
    
    ldw   sp,  0(r5)
    ldw   r1,  4(r5)
    ldw   r2,  8(r5)
    ldw   r3,  12(r5)
    ldw   r6,  16(r5)
    ldw   r7,  20(r5)
    ldw   r8,  24(r5)
    ldw   r9,  28(r5)
    ldw   r10,  32(r5)
    ldw   r11,  36(r5)
    ldw   r12,  40(r5)
    ldw   r13,  44(r5)
    ldw   r14,  48(r5)
    ldw   r15,  52(r5)
    ldw   r16,  56(r5)
    ldw   r17,  60(r5)
    ldw   r18,  64(r5)
    ldw   r19,  68(r5)
    ldw   r20,  72(r5)
    ldw   r21,  76(r5)
    ldw   r22,  80(r5)
    ldw   r23,  84(r5)
    ldw   fp,  88(r5)
    ldw   ea,  92(r5)
    ldw   ra,  96(r5)
    ldw   gp,  100(r5)
    ldw   r30, 108(r5)
        
    ret

