#include <defs.h>
#include <asm/mipsregs.h>
#include <clock.h>
#include <trap.h>
#include <arch.h>
#include <thumips_tlb.h>
#include <stdio.h>
#include <mmu.h>
#include <pmm.h>
#include <memlayout.h>
#include <glue_pgmap.h>
#include <assert.h>
#include <console.h>
#include <kdebug.h>
#include <error.h>
#include <syscall.h>
#include <proc.h>
#include <mips_io.h>

#define TICK_NUM 100

#define GET_CAUSE_EXCODE(x)   ( ((x) & CAUSEF_EXCCODE) >> CAUSEB_EXCCODE)

#define current (pls_read(current))
#define idleproc (pls_read(idleproc))

static void print_ticks()
{
	PRINT_HEX("%d ticks\n", TICK_NUM);
}

static const char *trapname(int trapno)
{
	static const char *const excnames[] = {
		"Interrupt",
		"TLB Modify",
		"TLB miss on load",
		"TLB miss on store",
		"Address error on load",
		"Address error on store",
		"Bus error on instruction fetch",
		"Bus error on data load or store",
		"Syscall",
		"Breakpoint",
		"Reserved (illegal) instruction",
		"Coprocessor unusable",
		"Arithmetic overflow",
	};
	if (trapno <= 12)
		return excnames[trapno];
	else
		return "Unknown";
}

bool trap_in_kernel(struct trapframe * tf)
{
	return !(tf->tf_status & KSU_USER);
}

void print_regs(struct pushregs *regs)
{
	int i;
	for (i = 0; i < 30; i++) {
		kprintf(" $");
		printbase10(i + 1);
		kprintf("\t: ");
		printhex(regs->reg_r[i]);
		kputchar('\n');
	}
}

void print_trapframe(struct trapframe *tf)
{
	PRINT_HEX("trapframe at ", tf);
	print_regs(&tf->tf_regs);
	PRINT_HEX(" $ra\t: ", tf->tf_ra);
	PRINT_HEX(" BadVA\t: ", tf->tf_vaddr);
	PRINT_HEX(" Status\t: ", tf->tf_status);
	PRINT_HEX(" Cause\t: ", tf->tf_cause);
	PRINT_HEX(" EPC\t: ", tf->tf_epc);
	if (!trap_in_kernel(tf)) {
		kprintf("Trap in usermode: ");
	} else {
		kprintf("Trap in kernel: ");
	}
	kprintf(trapname(GET_CAUSE_EXCODE(tf->tf_cause)));
	kputchar('\n');
}

static void interrupt_handler(struct trapframe *tf)
{
	extern clock_int_handler(void *);
	extern serial_int_handler(void *);
	int i;
	for (i = 0; i < 8; i++) {
		if (tf->tf_cause & (1 << (CAUSEB_IP + i))) {
			switch (i) {
			case TIMER0_IRQ:
				clock_int_handler(NULL);
				break;
			case COM1_IRQ:
				serial_int_handler(NULL);
				break;
			default:
				print_trapframe(tf);
				panic("Unknown interrupt!");
			}
		}
	}

}

extern pde_t *current_pgdir;

static inline int get_error_code(int write, pte_t * pte)
{
	int r = 0;
	if (pte != NULL && ptep_present(pte))
		r |= 0x01;
	if (write)
		r |= 0x02;
	return r;
}

static int
pgfault_handler(struct trapframe *tf, uint32_t addr, uint32_t error_code)
{
#if 0
	extern struct mm_struct *check_mm_struct;
	if (check_mm_struct != NULL) {
		return do_pgfault(check_mm_struct, error_code, addr);
	}
	panic("unhandled page fault.\n");
#endif
	extern struct mm_struct *check_mm_struct;
	if (check_mm_struct != NULL) {	//used for test check_swap
		//print_pgfault(tf);
	}
	struct mm_struct *mm;
	if (check_mm_struct != NULL) {
		assert(current == idleproc);
		mm = check_mm_struct;
	} else {
		if (current == NULL) {
			print_trapframe(tf);
			//print_pgfault(tf);
			panic("unhandled page fault.\n");
		}
		mm = current->mm;
	}			//kprintf("  (do_pgfault(%x,%d,%x))  ", mm, error_code, addr);
	return do_pgfault(mm, error_code, addr);
}

/* use software emulated X86 pgfault */
static void handle_tlbmiss(struct trapframe *tf, int write, int perm)
{
#if 0
	if (!trap_in_kernel(tf)) {
		print_trapframe(tf);
		while (1) ;
	}
#endif

	static int entercnt = 0;
	entercnt++;
	//kprintf("## enter handle_tlbmiss %d times\n", entercnt);
	int in_kernel = trap_in_kernel(tf);
	assert(current_pgdir != NULL);
	//print_trapframe(tf);
	uint32_t badaddr = tf->tf_vaddr;
	int ret = 0;
	pte_t *pte = get_pte(current_pgdir, tf->tf_vaddr, 0);
	if (perm || pte == NULL || ptep_invalid(pte) || (write && !ptep_u_write(pte))) {	//PTE miss, pgfault
		//panic("unimpl");
		//TODO
		//tlb will not be refill in do_pgfault,
		//so a vmm pgfault will trigger 2 exception
		//permission check in tlb miss
		ret = pgfault_handler(tf, badaddr, get_error_code(write, pte));
	} else {		//tlb miss only, reload it
		/* refill two slot */
		/* check permission */
		if (in_kernel) {
			tlb_refill(badaddr, pte);
			//kprintf("## refill K\n");
			return;
		} else {
			if (!ptep_u_read(pte)) {
				ret = -1;
				goto exit;
			}
			if (write && !ptep_u_write(pte)) {
				ret = -2;
				goto exit;
			}
			//kprintf("## refill U %d %08x\n", write, badaddr);
			tlb_refill(badaddr, pte);
			return;
		}
	}

exit:
	if (ret) {
		print_trapframe(tf);
		if (in_kernel) {
			panic("unhandled pgfault");
		} else {
			do_exit(-E_KILLED);
		}
	}
	return;
}

static void trap_dispatch(struct trapframe *tf)
{
	int i;
	int code = GET_CAUSE_EXCODE(tf->tf_cause);
	switch (code) {
	case EX_IRQ:
		interrupt_handler(tf);
		break;
	case EX_MOD:
		handle_tlbmiss(tf, 1, 1);
		break;
	case EX_TLBL:
		handle_tlbmiss(tf, 0, 0);
		break;
	case EX_TLBS:
		handle_tlbmiss(tf, 1, 0);
		break;
	case EX_RI:
		print_trapframe(tf);
		uint32_t *addr = (uint32_t *) (tf->tf_epc);
		for (i = 0; i < 10; ++i, addr++)
			kprintf("[%x:%x]\n", addr, *addr);

		panic("hey man! Do NOT use that insn! insn=%x",
		      *(uint32_t *) (tf->tf_epc));
		break;
	case EX_SYS:
		//print_trapframe(tf);
		tf->tf_epc += 4;
		syscall();
		break;
		/* alignment error or access kernel
		 * address space in user mode */
	case EX_ADEL:
	case EX_ADES:
		if (trap_in_kernel(tf)) {
			print_trapframe(tf);
			panic("Alignment Error");
		} else {
			print_trapframe(tf);
			do_exit(-E_KILLED);
		}
		break;
	default:
		print_trapframe(tf);
		panic("Unhandled Exception");
	}

}

/*
 * General trap (exception) handling function for mips.
 * This is called by the assembly-language exception handler once
 * the trapframe has been set up.
 */
void mips_trap(struct trapframe *tf)
{
	static int flag = 0;
	if (((tf->tf_cause >> 2) & 0x1F) != 0) {
//    kprintf(" [trap: epc=%x cause=%d syscall=%d badvaddr=%x pid=%d current_pgdir=%x] ", tf->tf_epc, (tf->tf_cause >> 2) & 0x1F, (unsigned)(tf->tf_regs.reg_r[MIPS_REG_V0]), tf->tf_vaddr, current?current->pid:-1, current_pgdir);
		flag = 1;
	} else
		flag = 0;

	// dispatch based on what type of trap occurred
	// used for previous projects
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
			if (current->flags & PF_EXITING) {
				do_exit(-E_KILLED);
			}
			if (current->need_resched) {
				schedule();
			}
		}
	}
//  if (flag) kprintf(" [end: epc=%x cause=%d syscall=%d pid=%d] ", tf->tf_epc, (tf->tf_cause >> 2) & 0x1F, (unsigned)(tf->tf_regs.reg_r[MIPS_REG_V0]), current?current->pid:-1);
}

//add by HHL
int ucore_in_interrupt()
{
	//panic("ucore_in_interrupt()");
	return 0;
}
