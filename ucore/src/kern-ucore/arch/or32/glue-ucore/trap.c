#include <trap.h>
#include <types.h>
#include <arch.h>
#include <uart.h>
#include <board.h>
#include <clock.h>
#include <stdio.h>
#include <picirq.h>
#include <intr.h>
#include <mmu.h>
#include <pmm.h>
#include <memlayout.h>
#include <proc.h>
#include <vmm.h>
#include <ide.h>
#include <syscall.h>
#include <error.h>
#include <sched.h>
#include <kio.h>

void bus_error_exception(struct trapframe *tf)
{
	print_trapframe(tf);
	kprintf("panic in assembly.\n");
	while (1) ;
}

static inline void print_pgfault(uintptr_t address, uint32_t error_code)
{
	/* error_code:
	 * bit 0 == 0 means no page found, 1 means protection fault
	 * bit 1 == 0 means read, 1 means write
	 * bit 2 == 0 means kernel, 1 means user
	 * */
	kprintf("page fault at 0x%08x: %c/%c [%s].\n", address,
		(error_code & 4) ? 'U' : 'K',
		(error_code & 2) ? 'W' : ((error_code & 8) ? 'E' : 'R'),
		(error_code & 1) ? "protection fault" : "no page found");
}

static int
do_pgmark(struct mm_struct *mm, uint32_t error_code, uintptr_t address)
{
	uint32_t write_acc = (error_code & 2) ? 1 : 0;
	struct vma_struct *vma = find_vma(mm, address);
	if (vma == NULL || vma->vm_start > address) {
		goto failed;
	}

	pte_t *ptep = get_pte(mm->pgdir, address, 0);
	if (ptep == 0) {
		kprintf("pte not exists\n");
		goto failed;
	}
	uint32_t write_perm = 0;
	if (error_code & 0x4) {	// User access
		write_perm = ptep_u_write(ptep);
	} else {		// Supervisor access
		write_perm = ptep_s_write(ptep);
	}
	if (write_acc && (vma->vm_flags & VM_WRITE) && !write_perm) {
		/* It's a copy-on-write page.
		 * call do_pgfault and check the return val. */
		//print_pgfault (address, error_code);
		if ((do_pgfault(mm, error_code, address)) != 0) {
			kprintf("shared memory handle failed\n");
			goto failed;
		}
	}

	ptep_set_accessed(ptep);
	ptep_set_perm(ptep, PTE_SPR_R | PTE_USER_R | PTE_E);
	if (write_acc) {
		ptep_set_dirty(ptep);
		ptep_set_perm(ptep, PTE_SPR_W | PTE_USER_W);
	}

	tlb_update(mm->pgdir, address);

	return 0;

failed:
	return -1;
}

int
do_page_fault(struct trapframe *tf, uintptr_t address, uint32_t vector,
	      uint32_t write_acc)
{
	int err;

	extern struct mm_struct *check_mm_struct;
	struct mm_struct *mm;
	if (check_mm_struct != NULL) {
		assert(pls_read(current) == pls_read(idleproc));
		mm = check_mm_struct;
	} else {
		if (pls_read(current) == NULL) {
			print_trapframe(tf);
			panic("unhandled page fault.\n");
		}
		mm = pls_read(current)->mm;
	}

	pte_t *ptep = get_pte(mm->pgdir, address, 0);
	/* We have to form the error code ourselves. */
	uint32_t error_code = 0;
	if (!(tf->sr & SPR_SR_SM))
		error_code |= 0x4;
	if (write_acc & 0x1)
		error_code |= 0x2;
	if (write_acc & 0x2)
		error_code |= 0x8;
	if (ptep == 0 || !(*ptep & PTE_P)) {	/* Normal page fault */
		//print_pgfault (address, error_code);
		err = do_pgfault(mm, error_code, address);
	} else {		/* Special faults for marking PTE_A and PTE_D */
		error_code |= 0x1;
		err = do_pgmark(mm, error_code, address);
	}

	return err;
}

void tick_timer_exception()
{
	ticks++;
	//if (ticks % 100 == 0)
	//      kprintf ("100 ticks\n");
	assert(pls_read(current) != NULL);
	run_timer_list();

	/* Tell the timer that we have done. */
	mtspr(SPR_TTMR, mfspr(SPR_TTMR) & (~SPR_TTMR_IP));
	mtspr(SPR_PICSR, mfspr(SPR_PICSR) & (~0x8));
}

/**
 * Get the current pending irq to be handled.
 * Copied from arch/or32/kernel/irq.c in or32-linux.
 */
static uint32_t pic_get_irq()
{
	int irq;
	int mask;

	unsigned long pend = mfspr(SPR_PICSR) & 0xfffffffc;

	if (pend & 0x0000ffff) {
		if (pend & 0x000000ff) {
			if (pend & 0x0000000f) {
				mask = 0x00000001;
				irq = 0;
			} else {
				mask = 0x00000010;
				irq = 4;
			}
		} else {
			if (pend & 0x00000f00) {
				mask = 0x00000100;
				irq = 8;
			} else {
				mask = 0x00001000;
				irq = 12;
			}
		}
	} else if (pend & 0xffff0000) {
		if (pend & 0x00ff0000) {
			if (pend & 0x000f0000) {
				mask = 0x00010000;
				irq = 16;
			} else {
				mask = 0x00100000;
				irq = 20;
			}
		} else {
			if (pend & 0x0f000000) {
				mask = 0x01000000;
				irq = 24;
			} else {
				mask = 0x10000000;
				irq = 28;
			}
		}
	} else {
		return -1;
	}

	while (!(mask & pend)) {
		mask <<= 1;
		irq++;
	}

//      mtspr(SPR_PICSR, mfspr(SPR_PICSR) & ~mask);
	return irq;
}

static uint32_t uart_irq_handler()
{
	extern void dev_stdin_write(char c);
	uint8_t c_iir;
	do {
		dev_stdin_write(uart_getc());
		//char c = uart_getc ();
		//kprintf ("got: %c[%x]\n", c, c);
		c_iir = REG8(UART_BASE + UART_IIR);
	} while ((c_iir & UART_IIR_NO_INT) == 0);

	mtspr(SPR_PICSR, mfspr(SPR_PICSR) & (~0x4));

	return 0;
}

void external_exception()
{
	uint32_t irq = pic_get_irq();
	switch (irq) {
	case IRQ_UART:
		uart_irq_handler();
		break;
	default:
		kprintf("Unknown irq: 0x%x\n", irq);
	}
}

bool trap_in_kernel(struct trapframe *tf)
{
	return (tf->sr & SPR_SR_SM);
}

void print_trapframe(struct trapframe *tf)
{
	kprintf("EPCR: %08x\n", tf->pc);
	kprintf("ESR: %08x\n", tf->sr);
	kprintf("EEAR: %08x\n", tf->ea);
	kprintf("GPR01: %08x\n", tf->sp);
	print_regs(&(tf->regs));
}

void print_regs(struct pushregs *regs)
{
	int i, j;
	for (i = 2, j = 1; i < 32; i++, j++) {
		kprintf("GPR%02d: %08x", i, regs->gprs[i - 2]);
		if (j < 4)
			kprintf("\t");
		else {
			kprintf("\n");
			j = 0;
		}
	}
	kprintf("\n");
}

void
trap_dispatch(struct trapframe *tf, uintptr_t address, uint32_t vector,
	      uint32_t write_acc)
{
	int ret;

	switch (vector) {
	case EXCEPTION_BUS:
		bus_error_exception(tf);
		break;
	case EXCEPTION_DPAGE_FAULT:
	case EXCEPTION_IPAGE_FAULT:
		if ((ret = do_page_fault(tf, address, vector, write_acc)) != 0) {
			print_trapframe(tf);
			if (pls_read(current) == NULL) {
				panic("handle pgfault failed %e.\n", ret);
			} else {
				if (trap_in_kernel(tf)) {
					panic
					    ("handle pgfault failed in kernel mode. %e\n",
					     ret);
				}
				kprintf("killed by kernel.\n");
				do_exit(-E_KILLED);
			}
		}
		break;
	case EXCEPTION_TIMER:
		tick_timer_exception();
		break;
	case EXCEPTION_EXTERNAL:
		external_exception();
		break;
	case EXCEPTION_SYSCALL:
		syscall();
		break;
	default:
		print_trapframe(tf);
		if (pls_read(current) != NULL) {
			kprintf("unhandled exception\n");
			do_exit(-E_KILLED);
		}
		panic("unhandled exception in kernel: 0x%x\n", vector);
	}
}

void
trap(struct trapframe *tf, uintptr_t address, uint32_t vector,
     uint32_t write_acc)
{
	tf->ea = address;
	if (pls_read(current) == NULL) {
		trap_dispatch(tf, address, vector, write_acc);
	} else {
		struct trapframe *otf = pls_read(current)->tf;
		pls_read(current)->tf = tf;

		bool in_kernel = trap_in_kernel(tf);

		trap_dispatch(tf, address, vector, write_acc);

		pls_read(current)->tf = otf;
		if (!in_kernel) {
			may_killed();
			if (pls_read(current)->need_resched) {
				schedule();
			}
		}
	}
}
