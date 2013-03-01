#ifndef __GLUE_INTR_H__
#define __GLUE_INTR_H__

#include <types.h>
#include <sync.h>

/* Trap Numbers */

#define T_TRAP                  3	//Trap instruction
#define T_UNIMPLE               4	//Unimplemented instruction
#define T_ILLEGAL               5	//Illegal instruction
#define T_MISALIGN_DATA         6	//Misaligned data address
#define T_MISALIGN_DEST         7	//Misaligned destination address
#define T_DIVISION              8	//Division error
#define T_SUPERONLY_INSADDR     9	//Supervisor-only instruction address
#define T_SUPERONLY_INS         10	//Supervisor-only instruction
#define T_SUPERONLY_DATA        11	//Supervisor-only data address
#define T_TLB_MISS              12	//Fast TLB miss or Double TLB miss
#define T_TLB_PERMISSION_EXE    13	//TLB permission violation (execute)
#define T_TLB_PERMISSION_READ   14	//TLB permission violation (read)
#define T_TLB_PERMISSION_WRITE  15	//TLB permission violation (write)

struct pushregs {
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;
	uint32_t r13;
	uint32_t r14;
	uint32_t r15;
	uint32_t r16;
	uint32_t r17;
	uint32_t r18;
	uint32_t r19;
	uint32_t r20;
	uint32_t r21;
	uint32_t r22;
	uint32_t r23;
};

struct trapframe {
	uint32_t ra;
	uint32_t ea;		// This becomes the PC once eret is executed
	struct pushregs tf_regs;
	uint32_t estatus;	//status before trap
	uint32_t fp;
	uint32_t sp;		//In user process, this is the pointer to user stack. The kernel stack sp is stored in context.
} __attribute__ ((packed));

void print_trapframe(struct trapframe *tf);
void print_regs(struct pushregs *regs);
bool trap_in_kernel(struct trapframe *tf);

//#define local_intr_enable_hw  do { __asm __volatile ("sti"); } while (0)
//#define local_intr_disable_hw do { __asm __volatile ("cli"); } while (0)

#define local_intr_save_hw(x)      local_intr_save(x)
#define local_intr_restore_hw(x)   local_intr_restore(x)

#endif /* !__GLUE_INTR_H__ */
