#include <asm/regdef.h>

.set noreorder

.text
.globl switch_to 
.type switch_to,@function
.ent switch_to 
switch_to:
/*
 * a0 contains a pointer to the old thread's struct pcb.
 * a1 contains a pointer to the new thread's struct pcb.
 *
 * The only thing we touch in the pcb is the first word, which
 * we save the stack pointer in. The other registers get saved
    * on the stack, namely:
    *
    *      s0-s8
    *      gp, ra
    *
    * The order must match arch/mips/include/switchframe.h.
    */

   /* Save the registers */
   sw	sp, 44(a0)
   sw	ra, 40(a0)
   sw	gp, 36(a0)
   sw	s8, 32(a0)
   sw	s7, 28(a0)
   sw	s6, 24(a0)
   sw	s5, 20(a0)
   sw	s4, 16(a0)
   sw	s3, 12(a0)
   sw	s2, 8(a0)
   sw	s1, 4(a0)
   sw	s0, 0(a0)

   nop

   /* Now, restore the registers */
   lw	s0, 0(a1)
   lw	s1, 4(a1)
   lw	s2, 8(a1)
   lw	s3, 12(a1)
   lw	s4, 16(a1)
   lw	s5, 20(a1)
   lw	s6, 24(a1)
   lw	s7, 28(a1)
   lw	s8, 32(a1)
   lw	gp, 36(a1)
   lw	ra, 40(a1)
   lw sp, 44(a1)
   nop			/* delay slot for load */

   /* and return. */
   j ra
   nop
   .end switch_to

