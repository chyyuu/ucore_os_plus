/*
 * Entry points for exceptions
 */
#include <asm/regdef.h>
#include <asm/mipsregs.h> 
#include <memlayout.h>
.extern pls_current # current pcb proc.h
.extern mips_trap
   /* 
    * Do not allow the assembler to use $1 (at), because we need to be
    * able to save it.
    */
   .set noat
   .set noreorder
   .section .text
# +0x000: R4000 tlbmiss vector (user)
.global ramExcHandle_tlbmiss
ramExcHandle_tlbmiss:
  #b ramExcHandle_tlbmiss
  b ramExcHandle_general 
  nop

.global ramReserved
ramReserved:
  b ramReserved
  nop

# +0x180: R4000 general vector
  .global ramExcHandle_general
  .type ramExcHandle_general, @function
  .ent ramExcHandle_general
ramExcHandle_general:
   move k1, sp			/* Save previous stack pointer in k1 */
   mfc0 k0, CP0_STATUS /* Get status register */
   andi k0, k0, KSU_USER/* Check the we-were-in-user-mode bit */
   beq	k0, $0, 1f		/* If clear, from kernel, already have stack */
   nop				/* delay slot */
   
   /* TODO */
   /* Coming from user mode - load kernel stack into sp */
   la k0, pls_current /* get address of "curkstack" */
   lw k0, 0(k0)
   lw k0, 20(k0)

   addiu sp, k0, KSTACKSIZE
   #lw sp, 72(k0)			/* get its value */
   nop				/* delay slot for the load */
  
1:
   mfc0 k0, CP0_CAUSE /* Now, load the exception cause. */
   j common_exception		/* Skip to common code */
   nop				/* delay slot */

ramExcHandle_general_end:
  .end ramExcHandle_general

/****************************************************/
/*                                                  */
/* Common exception code                            */
/*                                                  */
/****************************************************/
   
   .text
   .type common_exception,@function
   .ent common_exception
common_exception:	

   /*
    * At this point:
    *      Interrupts are off. (The processor did this for us.)
    *      k0 contains the exception cause value.
    *      k1 contains the old stack pointer.
    *      sp points into the kernel stack.
    *      All other registers are untouched.
    */
   
   /*
    * Allocate stack space for 37 words to hold the trap frame,
    * plus four more words for a minimal argument block.
    */
   addiu sp, sp, -164

   /* 
    * Save general registers.
    * We exclude k0/k1, which the kernel is free to clobber (and which
    * we already have clobbered), and $0, whose value is fixed.
    *
    * The order here must match mips/include/trapframe.h.
    *
    * gdb disassembles this code to try to figure out what registers
    * are where, and it isn't very bright. So in order to make gdb be
    * able to trace the stack back through here, we play some silly
    * games.
    *
    * In particular:
    *    (1) We store the return address register into the epc slot,
    *        which makes gdb think it's the return address slot. Then
    *        we store the real epc value over that.
    *    (2) We store the current sp into the sp slot, which makes gdb
    *        think it's the stack pointer slot. Then we store the real
    *        value.
    *    (3) gdb also assumes that saved registers in a function are
    *        saved in order. This is why we put epc where it is, and
    *        handle the real value of ra afterwards.
    *    (4) Because gdb will think we're saving k0 and k1, we need to
    *        leave slots for them in the trap frame, even though the
    *        stuff we save there is useless.
    */
   sw ra, 160(sp)	/* dummy for gdb */
   sw s8, 156(sp)	/* save s8 */
   sw sp, 152(sp)	/* dummy for gdb */
   sw gp, 148(sp)	/* save gp */
   sw k1, 144(sp)	/* dummy for gdb */
   sw k0, 140(sp)	/* dummy for gdb */

   sw k1, 152(sp)	/* real saved sp */
   nop			/* delay slot for store */
   
   mfc0 k1, CP0_EPC /* Copr.0 reg 13 == PC for exception */
   sw k1, 160(sp)	/* real saved PC */

   sw t9, 136(sp)
   sw t8, 132(sp)
   sw s7, 128(sp)
   sw s6, 124(sp)
   sw s5, 120(sp)
   sw s4, 116(sp)
   sw s3, 112(sp)
   sw s2, 108(sp)
   sw s1, 104(sp)
   sw s0, 100(sp)
   sw t7, 96(sp)
   sw t6, 92(sp)
   sw t5, 88(sp)
   sw t4, 84(sp)
   sw t3, 80(sp)
   sw t2, 76(sp)
   sw t1, 72(sp)
   sw t0, 68(sp)
   sw a3, 64(sp)
   sw a2, 60(sp)
   sw a1, 56(sp)
   sw a0, 52(sp)
   sw v1, 48(sp)
   sw v0, 44(sp)
   sw AT, 40(sp)

   sw ra, 36(sp)

   /*
    * Save special registers.
    */
   mfhi t0
   mflo t1
   sw t0, 32(sp)
   sw t1, 28(sp)

   /*
    * Save remaining exception context information.
    */

   sw   k0, 24(sp)               /* k0 was loaded with cause earlier */
   mfc0 t1, CP0_STATUS /* Copr.0 reg 11 == status */
   sw   t1, 20(sp)


   mfc0 t2, CP0_BADVADDR /* Copr.0 reg 8 == faulting vaddr */
   sw   t2, 16(sp)

   /*
    * Pretend to save $0 for gdb's benefit.
    */
   sw $0, 12(sp)

  # support nested interrupt
  la   t0, ~(ST0_KSU|ST0_EXL|ST0_IE)
  and t1, t1, t0
  mtc0 t1, CP0_STATUS /* Copr.0 reg 11 == status */

   /*
    * Prepare to call mips_trap(struct trapframe *)
    */

   addiu a0, sp, 16             /* set argument */
   la  t9, mips_trap
   jal t9 /* call it */
   nop				/* delay slot */

   /* Something must be here or gdb doesn't find the stack frame. */
   nop
   
   /*
    * Now restore stuff and return from the exception.
    * Interrupts should be off.
    */
exception_return:

   /*     16(sp)		   no need to restore tf_vaddr */
   lw t0, 20(sp)		/* load status register value into t0 */
   ori t0, t0, ST0_EXL
   nop				/* load delay slot */
   mtc0 t0, CP0_STATUS /* store it back to coprocessor 0 */
   /*     24(sp)		   no need to restore tf_cause */

   /* restore special registers */
   lw t1, 28(sp)
   lw t0, 32(sp)
   mtlo t1
   mthi t0

   /* load the general registers */
   lw ra, 36(sp)

   lw AT, 40(sp)
   lw v0, 44(sp)
   lw v1, 48(sp)
   lw a0, 52(sp)
   lw a1, 56(sp)
   lw a2, 60(sp)
   lw a3, 64(sp)
   lw t0, 68(sp)
   lw t1, 72(sp)
   lw t2, 76(sp)
   lw t3, 80(sp)
   lw t4, 84(sp)
   lw t5, 88(sp)
   lw t6, 92(sp)
   lw t7, 96(sp)
   lw s0, 100(sp)
   lw s1, 104(sp)
   lw s2, 108(sp)
   lw s3, 112(sp)
   lw s4, 116(sp)
   lw s5, 120(sp)
   lw s6, 124(sp)
   lw s7, 128(sp)
   lw t8, 132(sp)
   lw t9, 136(sp)

   /*     140(sp)		   "saved" k0 was dummy garbage anyway */
   /*     144(sp)		   "saved" k1 was dummy garbage anyway */

   lw gp, 148(sp)		/* restore gp */
   /*     152(sp)		   stack pointer - below */
   lw s8, 156(sp)		/* restore s8 */
   lw k0, 160(sp)		/* fetch exception return PC into k0 */

   lw sp, 152(sp)		/* fetch saved sp (must be last) */
   /* done */

   mtc0 k0, CP0_EPC /* Copr.0 reg 13 == PC for exception */
   nop
   eret
   nop
   .end common_exception 


.globl forkrets
.type forkrets, @function
.ent forkrets 
forkrets:
  addiu sp, a0, -16
  b exception_return
  nop

.end forkrets
