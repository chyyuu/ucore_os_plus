#include <types.h>
#include <mmu.h>
#include <memlayout.h>
#include <clock.h>
#include <trap.h>
#include <arm.h>
#include <stdio.h>
#include <assert.h>
#include <console.h>
#include <vmm.h>
#include <proc.h>
#include <sched.h>
#include <unistd.h>
#include <kio.h>
#include <picirq.h>
#include <syscall.h>
#include <error.h>
#include <signal.h>
#include <kgdb-stub.h>

#define TICK_NUM 5

//handle hw irq, should be implemented in mach_xxx
void irq_handler(void);
static int __irq_level = 0;

static const char *trapname(int trapno)
{
	static const char *const excnames[] = {
		"Reset",
		"Undefined instruction",
		"Software interrupt",
		"Prefetch abort",
		"Data abort",
		"Reserved",
		"Interrupt request",
		"Fast interrupt request"
	};

	if (trapno < sizeof(excnames) / sizeof(const char *const)) {
		return excnames[trapno];
	}
	if (trapno == T_SYSCALL) {
		return "System call";
	}
	if (trapno >= IRQ_OFFSET && trapno <= IRQ_OFFSET + IRQ_MAX_RANGE) {
		return "Hardware Interrupt";
	}
	return "(unknown trap)";
}

static const char *const modenames[] = {
	"User",
	"FIQ",
	"IRQ",
	"Supervisor",
	"", "", "",
	"Abort",
	"", "", "",
	"Undefined",
	"", "", "",
	"System"
};

/* trap_in_kernel - test if trap happened in kernel */
bool trap_in_kernel(struct trapframe *tf)
{
	return (tf->tf_sr & 0xF) != 0;
}

void print_trapframe(struct trapframe *tf)
{
	kprintf("trapframe at %p\n", tf);
	print_regs(&tf->tf_regs);
	kprintf("  usr_lr   0x%08x\n", tf->__tf_user_lr);
	kprintf("  sp   0x%08x\n", tf->tf_esp);
	kprintf("  lr   0x%08x\n", tf->tf_epc);
	kprintf("  spsr 0x%08x %s\n", tf->tf_sr, modenames[tf->tf_sr & 0xF]);
	kprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	kprintf("  err  0x%08x\n", tf->tf_err);
}

void print_regs(struct pushregs *regs)
{
	int i;
	for (i = 0; i < 11; i++) {
		kprintf("  r%02d  0x%08x\n", i, regs->reg_r[i]);
	}
	kprintf("  fp   0x%08x\n", regs->reg_r[11]);
	kprintf("  ip   0x%08x\n", regs->reg_r[12]);
}

static inline void print_pgfault(struct trapframe *tf)
{
	//print_trapframe(tf);
	uint32_t ttb = 0;
	asm volatile ("MRC p15, 0, %0, c2, c0, 0":"=r" (ttb));
	kprintf("%s page fault at (0x%08x) 0x%08x 0x%03x: %s-%s %s PID=%d\n",
		tf->tf_trapno == T_PABT ? "instruction" : tf->tf_trapno ==
		T_DABT ? "data" : "unknown", ttb, far(), tf->tf_err & 0xFFF,
		tf->tf_err & 0x2 ? "Page" : "Section",
		(tf->tf_err & 0xC) == 0xC ? "Permission" : (tf->tf_err & 0xC) ==
		0x8 ? "Domain" : (tf->tf_err & 0xC) ==
		0x4 ? "Translation" : "Alignment",
		//((fsr_v & 0xC) == 0) || ((fsr_v & 0xE) == 0x4) ? "Domain invalid" : "Domain valid",
		(tf->tf_err & 1 << 11) ? "W" : "R",
		current ? current->pid : -1);
	/* error_code:
	 * bit 0 == 0 means no page found, 1 means protection fault // translation or domain/permission
	 * bit 1 == 0 means read, 1 means write // permission
	 * bit 2 == 0 means kernel, 1 means user // can't know
	 * */
	//~ kprintf("page fault at 0x%08x: %c/%c [%s].\n", rcr2(),
	//~ (tf->tf_err & 4) ? 'U' : 'K',
	//~ (tf->tf_err & 2) ? 'W' : 'R',
	//~ (tf->tf_err & 1) ? "protection fault" : "no page found");
}

static int pgfault_handler(struct trapframe *tf)
{
	extern struct mm_struct *check_mm_struct;
	struct mm_struct *mm;
	if (check_mm_struct != NULL) {
		assert(current == idleproc);
		mm = check_mm_struct;
	} else {
		if (current == NULL) {
			print_trapframe(tf);
			print_pgfault(tf);
			panic("unhandled page fault.\n");
		}
		mm = current->mm;
	}
	//print_pgfault(tf);
	/* convert ARM error code to kernel error code */
	machine_word_t error_code = 0;
	if (tf->tf_err & (1 << 11))
		error_code |= 0x02;	//write
	if ((tf->tf_err & 0xC) != 0x04)
		error_code |= 0x01;
	uint32_t badaddr = 0;
	if (tf->tf_trapno == T_PABT) {
		badaddr = tf->tf_epc;
	} else {
		badaddr = far();
	}
	//kprintf("rrr %08x   %08x\n", error_code, *(volatile uint32_t*)(VPT_BASE+4*0xe00));
	return do_pgfault(mm, error_code, badaddr);
}

static void killed_by_kernel()
{
	kprintf("Process %d killed by kernel.\n",
		current ? current->pid : -1);
	do_exit(-E_KILLED);
}

static int udef_handler(struct trapframe *tf)
{
//  print_trapframe(tf);
	uint32_t inst = *(uint32_t *) (tf->tf_epc - 4);
	if (inst == KGDB_BP_INSTR) {
		return kgdb_trap(tf);
	} else {
		print_trapframe(tf);
		if (trap_in_kernel(tf)) {
			panic("undefined instruction\n");
		} else {
			killed_by_kernel();
		}
	}
	return 0;
}

/* trap_dispatch - dispatch based on what type of trap occurred */
static void trap_dispatch(struct trapframe *tf)
{
	char c;

	int ret;

	//kprintf("Trap [%03d %03d]\n", tf->tf_trapno, tf->tf_trapsubno);

	switch (tf->tf_trapno) {
		// Prefetch Abort service routine
		// Data Abort service routine
	case T_PABT:
	case T_DABT:
		if ((ret = pgfault_handler(tf)) != 0) {
			print_pgfault(tf);
			print_trapframe(tf);
			if (current == NULL) {
				panic("handle pgfault failed. %e\n", ret);
			} else {
				if (trap_in_kernel(tf)) {
					panic
					    ("handle pgfault failed in kernel mode. %e\n",
					     ret);
				}
				killed_by_kernel();
			}
		}
		break;
	case T_SWI:
		syscall();
		break;
		// IRQ Service Routine
		/*        case IRQ_OFFSET + INT_TIMER4:
		   ticks ++;
		   if (ticks % TICK_NUM == 0) {
		   print_ticks();
		   //print_trapframe(tf);
		   }
		   break;
		   case IRQ_OFFSET + INT_UART0:
		   c = cons_getc();
		   kprintf("serial [%03d] %c\n", c, c);
		   break;
		 */
		// SWI Service Routine
#if 0
	case T_SWITCH_TOK:	// a random System call
		kprintf("Random system call\n");
		print_cur_status();
		//print_stackframe();
		break;
#endif
	case T_IRQ:
		__irq_level++;
#if 0
		if (!trap_in_kernel(tf)) {
			uint32_t sp;
			asm volatile ("mov %0, sp":"=r" (sp));
			kprintf("### iRQnotK %08x\n", sp);
		}
#endif
		irq_handler();
		__irq_level--;
		break;
#if 0
	case T_PANIC:
		print_cur_status();
		//print_stackframe();
		break;
#endif
		/* for debugging */
	case T_UNDEF:
		udef_handler(tf);
		break;
	default:
		print_trapframe(tf);
		if (current != NULL) {
			kprintf("unhandled trap.\n");
			do_exit(-E_KILLED);
		}
		panic("unexpected trap in kernel.\n");
	}
}

static int __is_irq(struct trapframe *tf)
{
	return (tf->tf_trapno == T_IRQ);
}

/* *
 * trap - handles or dispatches an exception/interrupt. if and when trap() returns,
 * the code in kern/trap/trapentry.S will restore the state before the exception.
 * */
void trap(struct trapframe *tf)
{
	//print_trapframe(tf);
	if (current == NULL) {
		trap_dispatch(tf);
	} else {
		// keep a trapframe chain in stack
		struct trapframe *otf = current->tf;
		current->tf = tf;

		bool in_kernel = trap_in_kernel(tf);

		trap_dispatch(tf);

		current->tf = otf;
		if (!in_kernel) {
			may_killed();
			if (current->need_resched) {
				schedule();
			}
			do_signal(tf, NULL);
		}
	}
}

/* eabi compiler */
void raise(void)
{
	return;
}

int ucore_in_interrupt()
{
	return __irq_level;
}
