#by Chen Yuheng 2012/8

#include <trap.h>

	.macro	zero_fp
	mov	fp, #0
	.endm

#if 0
#define SPFIX(code...) code
#else
#define SPFIX(code...)
#endif

	.macro	svc_entry, stack_hole=0
	sub	sp, sp, #(S_FRAME_SIZE + \stack_hole)
 SPFIX(	tst	sp, #4		)
 SPFIX(	bicne	sp, sp, #4	)
	stmib	sp, {r1 - r12}

	ldmia	r0, {r1 - r3}
	add	r5, sp, #S_SP		@ here for interlock avoidance
	mov	r4, #-1			@  ""  ""      ""       ""
	add	r0, sp, #(S_FRAME_SIZE + \stack_hole)
 SPFIX(	addne	r0, r0, #4	)
	str	r1, [sp]		@ save the "real" r0 copied
					@ from the exception stack

	mov	r1, lr

	@
	@ We are now ready to fill in the remaining blanks on the stack:
	@
	@  r0 - sp_svc
	@  r1 - lr_svc
	@  r2 - lr_<exception>, already fixed up for correct return/restart
	@  r3 - spsr_<exception>
	@  r4 - orig_r0 (see pt_regs definition in ptrace.h)
	@
	stmia	r5, {r0 - r4}
	.endm

/*
 * User mode handlers
 *
 * EABI note: sp_svc is always 64-bit aligned here, so should S_FRAME_SIZE
 */

#if defined(CONFIG_AEABI) && (__LINUX_ARM_ARCH__ >= 5) && (S_FRAME_SIZE & 7)
#error "sizeof(struct pt_regs) must be a multiple of 8"
#endif

	.macro	usr_entry, stack_hole=0
	sub	sp, sp, #(S_FRAME_SIZE+\stack_hole)
	stmib	sp, {r1 - r12}

	ldmia	r0, {r1 - r3}
	add	r0, sp, #S_PC		@ here for interlock avoidance
	mov	r4, #-1			@  ""  ""     ""        ""

	str	r1, [sp]		@ save the "real" r0 copied
					@ from the exception stack

	@
	@ We are now ready to fill in the remaining blanks on the stack:
	@
	@  r2 - lr_<exception>, already fixed up for correct return/restart
	@  r3 - spsr_<exception>
	@  r4 - orig_r0 (see pt_regs definition in ptrace.h)
	@
	@ Also, separately save sp_usr and lr_usr
	@
	stmia	r0, {r2 - r4}
	stmdb	r0, {sp, lr}^

	@
	@ Enable the alignment trap while in kernel mode
	@
	@alignment_trap r0

	@
	@ Clear FP to mark the first stack frame
	@
	zero_fp
	.endm

  .macro do_trap, trapno
  mov r0, #\trapno
  str r0, [sp, #S_TF_TRAPNO]
  mov r0, sp
  bl trap
  .endm

  .macro svc_return
	ldr	r0, [sp, #S_PSR]
	msr	spsr_cxsf, r0
	ldmia	sp, {r0 - pc}^			@ load r0 - pc, cpsr
  .endm


# vectors.S sends all traps here.
.text
.global __switrap
__switrap:
	# Mode: SWI, I: disabled
	
	# link register offset don't change
	# Only valid for SWI
	sub	sp, sp, #S_FRAME_SIZE
	stmia	sp, {r0 - r12}			@ Calling r0 - r12
	add	r8, sp, #S_PC
	stmdb	r8, {sp, lr}^			@ Calling sp, lr
	mrs	r8, spsr			@ called from non-FIQ mode, so ok.
	str	lr, [sp, #S_PC]			@ Save calling PC
	str	r8, [sp, #S_PSR]		@ Save CPSR
	str	r0, [sp, #S_OLD_R0]		@ Save OLD_R0
	zero_fp

  # get syscall no
  ldr r0, [lr, #-4]
	# Mask off top 8 bits
	bic r0, r0, #0xff000000
  str r0, [sp, #S_TF_ERR]

  do_trap T_SWI
  b ret_to_user

.macro read_dabt_fsr
#if __LINUX_ARM_ARCH__ >= 7
/* Error code: fsr for dabt */
/* c5[11] == r/w */
  mrc p15, 0, r0, c5, c0, 0
  str   r0, [sp, #S_TF_ERR]
#else
  MRC p15, 0, r1, c5, c0, 1
  ldr r0, [sp, #S_PC]
  ldr r0, [r0]
  bic r1, r1, #( (1<<10)|(1<<11) )
  tst r0, #(1<<20)
  orreq r1, r1, #(1<<11)
  str   r1, [sp, #S_TF_ERR]
#endif
.endm

.macro read_pabt_fsr
/* Error code: fsr for prefetch */
  MRC p15, 0, r0, c5, c0, 1
  str   r0, [sp, #S_TF_ERR]
.endm

	.align	5
__dabt_svc:
	svc_entry
  read_dabt_fsr
  do_trap T_DABT
  svc_return


__dabt_usr:
  usr_entry
  read_dabt_fsr
  do_trap T_DABT
  b ret_to_user

__pabt_svc:
  svc_entry
  read_pabt_fsr
  do_trap T_PABT
  svc_return

__pabt_usr:
  usr_entry
  read_pabt_fsr
  do_trap T_PABT
  b ret_to_user

__und_svc:
  svc_entry
  do_trap T_UNDEF
  svc_return

__und_usr:
	usr_entry
  do_trap T_UNDEF
  b ret_to_user

__irq_svc:
  svc_entry
  do_trap T_IRQ
  svc_return

__irq_usr:
  usr_entry
  do_trap T_IRQ
  b ret_to_user

__irq_invalid:
  b __irq_invalid

__dabt_invalid:
  b __dabt_invalid

__pabt_invalid:
  b __pabt_invalid

__und_invalid:
  b __und_invalid


/*
 * Vector stubs.
 *
 * This code is copied to 0xffff0200 so we can use branches in the
 * vectors, rather than ldr's.  Note that this code must not
 * exceed 0x300 bytes.
 *
 * Common stub entry macro:
 *   Enter in IRQ mode, spsr = SVC/USR CPSR, lr = SVC/USR PC
 *
 * SP points to a minimal amount of processor-private memory, the address
 * of which is copied into r0 for the mode specific abort handler.
 */
	.macro	vector_stub, name, mode, correction=0
  .align 5
.globl real_vector_\name
real_vector_\name:
	.if \correction
	sub	lr, lr, #\correction
	.endif

	@
	@ Save r0, lr_<exception> (parent PC) and spsr_<exception>
	@ (parent CPSR)
	@
	stmia	sp, {r0, lr}		@ save r0, lr
	mrs	lr, spsr
	str	lr, [sp, #8]		@ save spsr

	@
	@ Prepare for SVC32 mode.  IRQs remain disabled.
	@
	mrs	r0, cpsr
	eor	r0, r0, #(\mode ^ SVC_MODE)
	msr	spsr_cxsf, r0

	@
	@ the branch table must immediately follow this code
	@
	and	lr, lr, #0x0f
	mov	r0, sp
	ldr	lr, [pc, lr, lsl #2]
	movs	pc, lr			@ branch to handler in SVC mode
	.endm

@	.globl	__stubs_start
__stubs_start:
/*
 * Interrupt dispatcher
 */
	vector_stub	irq, IRQ_MODE, 4

	.long	__irq_usr			@  0  (USR_26 / USR_32)
	.long	__irq_invalid			@  1  (FIQ_26 / FIQ_32)
	.long	__irq_invalid			@  2  (IRQ_26 / IRQ_32)
	.long	__irq_svc			@  3  (SVC_26 / SVC_32)
	.long	__irq_invalid			@  4
	.long	__irq_invalid			@  5
	.long	__irq_invalid			@  6
	.long	__irq_invalid			@  7
	.long	__irq_invalid			@  8
	.long	__irq_invalid			@  9
	.long	__irq_invalid			@  a
	.long	__irq_invalid			@  b
	.long	__irq_invalid			@  c
	.long	__irq_invalid			@  d
	.long	__irq_invalid			@  e
	.long	__irq_invalid			@  f

/*
 * Data abort dispatcher
 * Enter in ABT mode, spsr = USR CPSR, lr = USR PC
 */
	vector_stub	dabt, ABT_MODE, 8

	.long	__dabt_usr			@  0  (USR_26 / USR_32)
	.long	__dabt_invalid			@  1  (FIQ_26 / FIQ_32)
	.long	__dabt_invalid			@  2  (IRQ_26 / IRQ_32)
	.long	__dabt_svc			@  3  (SVC_26 / SVC_32)
	.long	__dabt_invalid			@  4
	.long	__dabt_invalid			@  5
	.long	__dabt_invalid			@  6
	.long	__dabt_invalid			@  7
	.long	__dabt_invalid			@  8
	.long	__dabt_invalid			@  9
	.long	__dabt_invalid			@  a
	.long	__dabt_invalid			@  b
	.long	__dabt_invalid			@  c
	.long	__dabt_invalid			@  d
	.long	__dabt_invalid			@  e
	.long	__dabt_invalid			@  f

/*
 * Prefetch abort dispatcher
 * Enter in ABT mode, spsr = USR CPSR, lr = USR PC
 */
	vector_stub	pabt, ABT_MODE, 4

	.long	__pabt_usr			@  0 (USR_26 / USR_32)
	.long	__pabt_invalid			@  1 (FIQ_26 / FIQ_32)
	.long	__pabt_invalid			@  2 (IRQ_26 / IRQ_32)
	.long	__pabt_svc			@  3 (SVC_26 / SVC_32)
	.long	__pabt_invalid			@  4
	.long	__pabt_invalid			@  5
	.long	__pabt_invalid			@  6
	.long	__pabt_invalid			@  7
	.long	__pabt_invalid			@  8
	.long	__pabt_invalid			@  9
	.long	__pabt_invalid			@  a
	.long	__pabt_invalid			@  b
	.long	__pabt_invalid			@  c
	.long	__pabt_invalid			@  d
	.long	__pabt_invalid			@  e
	.long	__pabt_invalid			@  f

/*
 * Undef instr entry dispatcher
 * Enter in UND mode, spsr = SVC/USR CPSR, lr = SVC/USR PC
 */
	vector_stub	und, UND_MODE

	.long	__und_usr			@  0 (USR_26 / USR_32)
	.long	__und_invalid			@  1 (FIQ_26 / FIQ_32)
	.long	__und_invalid			@  2 (IRQ_26 / IRQ_32)
	.long	__und_svc			@  3 (SVC_26 / SVC_32)
	.long	__und_invalid			@  4
	.long	__und_invalid			@  5
	.long	__und_invalid			@  6
	.long	__und_invalid			@  7
	.long	__und_invalid			@  8
	.long	__und_invalid			@  9
	.long	__und_invalid			@  a
	.long	__und_invalid			@  b
	.long	__und_invalid			@  c
	.long	__und_invalid			@  d
	.long	__und_invalid			@  e
	.long	__und_invalid			@  f

ret_to_user:
	@ slow_restore_user_regs
	ldr	r1, [sp, #S_PSR]		@ get calling cpsr
	ldr	lr, [sp, #S_PC]!		@ get pc
	msr	spsr_cxsf, r1			@ save in spsr_svc
	ldmdb	sp, {r0 - lr}^			@ get calling r0 - lr
	mov	r0, r0
	add	sp, sp, #S_FRAME_SIZE - S_PC
	movs	pc, lr				@ return & move spsr_svc into cpsr

# r0 == new tf
.globl forkrets
forkrets:
  # set stack to this new process's trapframe
	mov sp, r0
  ldr r1, [sp, #S_PSR]
  and r1, r1, #0xF
  cmp r1, #0
  beq ret_to_user
  svc_return

