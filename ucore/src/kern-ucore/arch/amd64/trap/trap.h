#ifndef __KERN_TRAP_TRAP_H__
#define __KERN_TRAP_TRAP_H__

#include <types.h>
#include <arch.h>

/* Trap Numbers */
#define T_SYSCALL 0x80
#define T_IPI     0x81
#define T_IPI_DOS 0x82

/* Processor-defined: */
#define T_DIVIDE                0	// divide error
#define T_DEBUG                 1	// debug exception
#define T_NMI                   2	// non-maskable interrupt
#define T_BRKPT                 3	// breakpoint
#define T_OFLOW                 4	// overflow
#define T_BOUND                 5	// bounds check
#define T_ILLOP                 6	// illegal opcode
#define T_DEVICE                7	// device not available
#define T_DBLFLT                8	// double fault
// #define T_COPROC             9   // reserved (not used since 486)
#define T_TSS                   10	// invalid task switch segment
#define T_SEGNP                 11	// segment not present
#define T_STACK                 12	// stack exception
#define T_GPFLT                 13	// general protection fault
#define T_PGFLT                 14	// page fault
// #define T_RES                15  // reserved
#define T_FPERR                 16	// floating point error
#define T_ALIGN                 17	// aligment check
#define T_MCHK                  18	// machine check
#define T_SIMDERR               19	// SIMD floating point error

/* Hardware IRQ numbers. We receive these as (IRQ_OFFSET + IRQ_xx) */
#define IRQ_OFFSET              32	// IRQ 0 corresponds to int IRQ_OFFSET
#define IRQ_COUNT               32

#define IRQ_TIMER               0
#define IRQ_KBD                 1
#define IRQ_COM1                4
#define IRQ_LPT1                7
#define IRQ_IDE1                14
#define IRQ_IDE2                15
#define IRQ_ERROR               19
#define IRQ_SPURIOUS            31

// These are arbitrarily chosen, but with care not to overlap
// processor defined exceptions or interrupt vectors.
#define T_TLBFLUSH      65      // flush TLB
#define T_SAMPCONF      66      // configure event counters
#define T_IPICALL       67      // Queued IPI call
#define T_DEFAULT      500      // catchall



/* registers as pushed by pushal */
struct pushregs {
	uint64_t reg_r15;
	uint64_t reg_r14;
	uint64_t reg_r13;
	uint64_t reg_r12;
	uint64_t reg_rbp;
	uint64_t reg_rbx;
	uint64_t reg_r11;
	uint64_t reg_r10;
	uint64_t reg_r9;
	uint64_t reg_r8;
	uint64_t reg_rax;
	uint64_t reg_rcx;
	uint64_t reg_rdx;
	uint64_t reg_rsi;
	uint64_t reg_rdi;
};

struct trapframe {
	uint16_t tf_ds;
	uint16_t tf_padding0[3];
	uint16_t tf_es;
	uint16_t tf_padding1[3];
	struct pushregs tf_regs;
	uint64_t tf_trapno;
	/* below here defined by x86 hardware */
	uint64_t tf_err;
	uintptr_t tf_rip;
	uint16_t tf_cs;
	uint16_t tf_padding2[3];
	uint64_t tf_rflags;
	uintptr_t tf_rsp;
	uint16_t tf_ss;
	uint16_t tf_padding3[3];
} __attribute__ ((packed));

void idt_init(void);
void print_trapframe(struct trapframe *tf);
void print_regs(struct pushregs *regs);
bool trap_in_kernel(struct trapframe *tf);

#define local_intr_enable_hw  do { __asm __volatile ("sti"); } while (0)
#define local_intr_disable_hw do { __asm __volatile ("cli"); } while (0)

#define local_intr_save_hw(x)      do { (x) = (read_rflags() & FL_IF) != 0; cli(); } while (0)
#define local_intr_restore_hw(x)   do { if (x) sti(); } while (0)

#endif /* !__KERN_TRAP_TRAP_H__ */
