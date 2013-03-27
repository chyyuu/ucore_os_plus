#ifndef __KERN_TRAP_TRAP_H__
#define __KERN_TRAP_TRAP_H__

/* Trap Numbers */

/* Processor-defined: */
#define T_RESET			0	// Reset
#define T_UNDEF			1	// Undefined instruction
#define T_SWI			2	// software interrupt
#define T_PABT			3	// Prefetch abort
#define T_DABT			4	// Data abort
// #define T_RESERVED
#define T_IRQ			6	// Interrupt request
#define T_FIQ			7	// Fast interrupt request

/* *
 * Hardware interrupt
 * ranges from 32 to 63
 * */
#define IRQ_OFFSET		32	// Interrupt request
#define IRQ_MAX_RANGE	63

#define USR26_MODE	0x00000000
#define FIQ26_MODE	0x00000001
#define IRQ26_MODE	0x00000002
#define SVC26_MODE	0x00000003
#define USR_MODE	0x00000010
#define FIQ_MODE	0x00000011
#define IRQ_MODE	0x00000012
#define SVC_MODE	0x00000013
#define ABT_MODE	0x00000017
#define UND_MODE	0x0000001b

/* *
 * These are arbitrarily chosen, but with care not to overlap
 * processor defined exceptions or interrupt vectors.
 * */
#define T_SWITCH_TOK            121	// a random system call
#define T_PANIC            122	// a random system call

#define S_TF_TRAPNO  72
#define S_TF_ERR     76

#define S_FRAME_SIZE	(72+8)
#define S_OLD_R0	68
#define S_PSR		64

#define S_PC		60
#define S_LR		56
#define S_SP		52
#define S_IP		48
#define S_FP		44
#define S_R10		40
#define S_R9		36
#define S_R8		32
#define S_R7		28
#define S_R6		24
#define S_R5		20
#define S_R4		16
#define S_R3		12
#define S_R2		8
#define S_R1		4
#define S_R0		0
#define S_OFF		8

#ifndef __ASSEMBLER__

#include <types.h>
#include <arch.h>
#include <intr.h>
#include <sync.h>

#if 0
/* General purpose registers minus fp, sp and pc */
struct pushregs {
	uint32_t reg_r[13];
};
#endif

/*
 * This struct defines the way the registers are stored on the
 * stack during a system call.  Note that sizeof(struct pt_regs)
 * has to be a multiple of 8.
 */
struct pushregs {
	long reg_r[18];
};

#define ARM_cpsr	reg_r[16]
#define ARM_pc		reg_r[15]
#define ARM_lr		reg_r[14]
#define ARM_sp		reg_r[13]
#define ARM_ip		reg_r[12]
#define ARM_fp		reg_r[11]
#define ARM_r10		reg_r[10]
#define ARM_r9		reg_r[9]
#define ARM_r8		reg_r[8]
#define ARM_r7		reg_r[7]
#define ARM_r6		reg_r[6]
#define ARM_r5		reg_r[5]
#define ARM_r4		reg_r[4]
#define ARM_r3		reg_r[3]
#define ARM_r2		reg_r[2]
#define ARM_r1		reg_r[1]
#define ARM_r0		reg_r[0]
#define ARM_ORIG_r0	reg_r[17]

#define user_mode(regs)	\
	(((regs)->ARM_cpsr & 0xf) == 0)

#define thumb_mode(regs) (0)

#define isa_mode(regs) \
	((((regs)->ARM_cpsr & PSR_J_BIT) >> 23) | \
	 (((regs)->ARM_cpsr & PSR_T_BIT) >> 5))

#define processor_mode(regs) \
	((regs)->ARM_cpsr & MODE_MASK)

#define interrupts_enabled(regs) \
	(!((regs)->ARM_cpsr & PSR_I_BIT))

#define fast_interrupts_enabled(regs) \
	(!((regs)->ARM_cpsr & PSR_F_BIT))

#define tf_epc tf_regs.ARM_pc
#define tf_sr  tf_regs.ARM_cpsr
#define tf_esp tf_regs.ARM_sp
#define __tf_user_lr tf_regs.ARM_lr

/* Trapframe structure
 *   Structure built in the exception stack
 *   containing information on the nature of
 *   the occured exception.
 * */
struct trapframe {
	struct pushregs tf_regs;
	uint32_t tf_trapno;	// Trap number
	uint32_t tf_err;	// Error code
} __attribute__ ((packed));

#define local_intr_enable_hw  intr_enable()
#define local_intr_disable_hw intr_disable()

#define local_intr_save_hw(x)      local_intr_save(x)
#define local_intr_restore_hw(x)   local_intr_restore(x)

void print_trapframe(struct trapframe *tf);
void print_regs(struct pushregs *regs);
bool trap_in_kernel(struct trapframe *tf);
int ucore_in_interrupt();

#endif /* !__ASSEMBLER__ */

#endif /* !__KERN_TRAP_TRAP_H__ */
