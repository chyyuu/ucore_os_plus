#ifndef _MIPS_TRAPFRAME_H_
#define _MIPS_TRAPFRAME_H_

/* $1 - $30 */
struct pushregs {
	uint32_t reg_r[30];
};

#define MIPS_REG_START  (0)
#define MIPS_REG_AT    (MIPS_REG_START+0)
#define MIPS_REG_V0    (MIPS_REG_START+1)
#define MIPS_REG_V1    (MIPS_REG_START+2)
#define MIPS_REG_A0    (MIPS_REG_START+3)
#define MIPS_REG_A1    (MIPS_REG_START+4)
#define MIPS_REG_A2    (MIPS_REG_START+5)
#define MIPS_REG_A3    (MIPS_REG_START+6)
#define MIPS_REG_T0    (MIPS_REG_START+7)
#define MIPS_REG_T1    (MIPS_REG_START+8)
#define MIPS_REG_T2    (MIPS_REG_START+9)
#define MIPS_REG_T3    (MIPS_REG_START+10)
#define MIPS_REG_T4    (MIPS_REG_START+11)
#define MIPS_REG_T5    (MIPS_REG_START+12)
#define MIPS_REG_T6    (MIPS_REG_START+13)
#define MIPS_REG_T7    (MIPS_REG_START+14)

#define MIPS_REG_S0    (MIPS_REG_START+15)
#define MIPS_REG_S1    (MIPS_REG_START+16)
#define MIPS_REG_S2    (MIPS_REG_START+17)
#define MIPS_REG_S3    (MIPS_REG_START+18)
#define MIPS_REG_S4    (MIPS_REG_START+19)
#define MIPS_REG_S5    (MIPS_REG_START+20)
#define MIPS_REG_S6    (MIPS_REG_START+21)
#define MIPS_REG_S7    (MIPS_REG_START+22)

#define MIPS_REG_T8    (MIPS_REG_START+23)
#define MIPS_REG_T9    (MIPS_REG_START+24)

#define MIPS_REG_GP    (MIPS_REG_START+27)
#define MIPS_REG_SP    (MIPS_REG_START+28)
#define MIPS_REG_FP    (MIPS_REG_START+29)

/*
 * Structure describing what is saved on the stack during entry to
 * the exception handler.
 *
 * This must agree with the code in exception.S.
 */

struct trapframe {
	uint32_t tf_vaddr;	/* coprocessor 0 vaddr register */
	uint32_t tf_status;	/* coprocessor 0 status register */
	uint32_t tf_cause;	/* coprocessor 0 cause register */
	uint32_t tf_lo;
	uint32_t tf_hi;
	uint32_t tf_ra;		/* Saved register 31 */
	struct pushregs tf_regs;
	uint32_t tf_epc;	/* coprocessor 0 epc register */
};

/*
 * MIPS exception codes.
 */
#define EX_IRQ    0		/* Interrupt */
#define EX_MOD    1		/* TLB Modify (write to read-only page) */
#define EX_TLBL   2		/* TLB miss on load */
#define EX_TLBS   3		/* TLB miss on store */
#define EX_ADEL   4		/* Address error on load */
#define EX_ADES   5		/* Address error on store */
#define EX_IBE    6		/* Bus error on instruction fetch */
#define EX_DBE    7		/* Bus error on data load *or* store */
#define EX_SYS    8		/* Syscall */
#define EX_BP     9		/* Breakpoint */
#define EX_RI     10		/* Reserved (illegal) instruction */
#define EX_CPU    11		/* Coprocessor unusable */
#define EX_OVF    12		/* Arithmetic overflow */

#endif /* _MIPS_TRAPFRAME_H_ */
