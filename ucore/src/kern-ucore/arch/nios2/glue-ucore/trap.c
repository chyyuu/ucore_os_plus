#include <trap.h>
#include <nios2.h>
//#include <defs.h>
#include <mmu.h>
#include <memlayout.h>
#include <clock.h>
#include <kio.h>
#include <assert.h>
#include <vmm.h>
#include <unistd.h>
#include <syscall.h>
#include <error.h>
#include <sched.h>
#include <sync.h>
#include <arch.h>
#include <system.h>
#include <altera_avalon_timer_regs.h>
#include <proc.h>
#include <console.h>
#include <nios2_timer.h>
#include <rf212.h>

#define current (pls_read(current))

void forkrets(struct trapframe *tf);

void inline print_regs(struct pushregs *regs)
{
	kprintf("  r23  0x%08x\n", regs->r23);
	kprintf("  r22  0x%08x\n", regs->r22);
	kprintf("  r21  0x%08x\n", regs->r21);
	kprintf("  r20  0x%08x\n", regs->r20);
	kprintf("  r19  0x%08x\n", regs->r19);
	kprintf("  r18  0x%08x\n", regs->r18);
	kprintf("  r17  0x%08x\n", regs->r17);
	kprintf("  r16  0x%08x\n", regs->r16);
	kprintf("  r15  0x%08x\n", regs->r15);
	kprintf("  r14  0x%08x\n", regs->r14);
	kprintf("  r13  0x%08x\n", regs->r13);
	kprintf("  r12  0x%08x\n", regs->r12);
	kprintf("  r11  0x%08x\n", regs->r11);
	kprintf("  r10  0x%08x\n", regs->r10);
	kprintf("  r9  0x%08x\n", regs->r9);
	kprintf("  r8  0x%08x\n", regs->r8);
	kprintf("  r7  0x%08x\n", regs->r7);
	kprintf("  r6  0x%08x\n", regs->r6);
	kprintf("  r5  0x%08x\n", regs->r5);
	kprintf("  r4  0x%08x\n", regs->r4);
	kprintf("  r3  0x%08x\n", regs->r3);
	kprintf("  r2  0x%08x\n", regs->r2);
	kprintf("  r1  0x%08x\n", regs->r1);
}

void inline print_trapframe(struct trapframe *tf)
{
	kprintf("trapframe at %p\n", tf);
	kprintf("  sp  0x%08x\n", tf->sp);
	kprintf("  fp  0x%08x\n", tf->fp);
	kprintf("  estatus  0x%08x\n", tf->estatus);
	print_regs(&tf->tf_regs);
	kprintf("  ea  0x%08x\n", tf->ea);
	kprintf("  ra  0x%08x\n", tf->ra);

	/*
	   pte_t *ptep = get_pte(NIOS2_PGDIR, current->tf->ea, 0);
	   if (ptep != NULL) {
	   int i;
	   for (i = -20; i <= 20; i += 4) {
	   uint32_t *addr = (uint32_t*)((0xFFFFF000 & (int)ptep) + (current->tf->ea & 0xFFF) + i);
	   kprintf("0x%08x (0x%08x): 0x%08x\n", current->tf->ea + i, addr, *addr);
	   }
	   }
	 */
}

bool trap_in_kernel(struct trapframe *tf)
{
	return !(tf->estatus & NIOS2_STATUS_U_MSK);
}

static inline int irq_dispatch(struct trapframe *tf)
{
	int irq_count = 0;
	char c;
	if (tf->estatus & NIOS2_STATUS_PIE_MSK) {
		uint32_t ipending;
		NIOS2_READ_IPENDING(ipending);
		while (ipending) {
			uint32_t mask = 1;
			uint32_t i = 0;
			while (!(ipending & mask)) {
				mask <<= 1;
				i++;
			}
			++irq_count;
			switch (i) {
			case TIMER_IRQ:
				IOWR_ALTERA_AVALON_TIMER_STATUS(na_timer, 0);
				ticks++;

//                if (ticks % 1000 == 0)
//                    kprintf("\t\t\t\t[timer: nr_free_pages()=%d]\n", nr_free_pages());
//                    kprintf("100 ticks\n");

				assert(current != NULL);
				run_timer_list();
				break;
			case JTAG_UART_IRQ:
				while ((c = cons_getc()) != 0) {
					extern void dev_stdin_write(char c);
					dev_stdin_write(c);
				}
				break;
			case RF212_0_IRQ:
				rf212_int_handler();
				break;
			default:
				panic("unknown irq %d\n", i);
				break;
			}
			NIOS2_READ_IPENDING(ipending);
		}
	}
	return irq_count;
}

/*
static uint32_t
getaddr(struct mm_struct *mm, uint32_t la) {
    pte_t pgdir = mm->pgdir;
    pte_t *ptep = get_pte(pgdir, la, 0);
    return 0xC0000000 + (*ptep & 0xFFFFF000) | (la & 0xFFF);
}
static void
db_print(struct mm_struct *mm, uint32_t sp) {
    if (!mm) return;
    kprintf("PRINT::: ");
    pte_t pgdir = mm->pgdir;
    kprintf("pgdir=0x%x\n", pgdir);
    int i;
    for (i = 0; i < 20; ++i) {
        uint32_t la = sp + i * 4;
        uint32_t va = getaddr(mm, la);
        kprintf("\t+%d: [0x%08x->0x%08x]:0x%08x\n", i*4, la, va, *((uint32_t*)va));
    }
}
*/
static inline void exception_dispatch(struct trapframe *tf)
{
	uint32_t cause, badaddr, pteaddr;
	NIOS2_READ_EXCEPTION(cause);
	cause = (cause >> 2) & 0x1F;

	NIOS2_READ_BADADDR(badaddr);
	NIOS2_READ_PTEADDR(pteaddr);

	switch (cause) {
	case 0:		//NO TRAP
		break;
	case T_TRAP:		//syscall
		tf->ea += 4;
		syscall();
		break;
	case T_UNIMPLE:	//Unimplemented instruction
		panic("Unimplemented instruction! ea=0x%x\n", tf->ea);
		break;
	case T_ILLEGAL:	//Illegal instruction
		print_trapframe(tf);
		panic("Illegal instruction! cause=%d badaddr=0x%x\n", cause,
		      badaddr);
		break;
	case T_MISALIGN_DATA:	//Misaligned data address
		print_trapframe(tf);
		panic("Misaligned data address! ea=0x%x badaddr=0x%x\n", tf->ea,
		      badaddr);
		break;
	case T_MISALIGN_DEST:	//Misaligned destination address
		print_trapframe(tf);
		//db_print(current->mm, tf->sp - 8);
		panic("Misaligned destination address! ea=0x%x, badaddr=0x%x\n",
		      tf->ea, badaddr);
		break;
	case T_DIVISION:	//Division error
		print_trapframe(tf);

		panic("Division error! ea=0x%x\n", tf->ea);
		break;
	case T_SUPERONLY_INSADDR:	//Supervisor-only instruction address
		panic("Supervisor-only instruction address! ea=0x%x\n", tf->ea);
		break;
	case T_SUPERONLY_INS:	//Supervisor-only instruction
		panic("Supervisor-only instruction! ea=0x%x\n", tf->ea);
		break;
	case T_SUPERONLY_DATA:	//Supervisor-only data address
		print_trapframe(current->tf);
		panic("Supervisor-only data address! badaddr=0x%x ea=0x%x\n",
		      badaddr, tf->ea);
		break;
	case T_TLB_MISS:	//Fast TLB miss or Double TLB miss
		//kprintf("tlb_miss: tf->sp=0x%x\n", tf->sp);
		if (tlb_miss_handler(((pteaddr >> 2) & 0xFFFFF) << 12, 0) != 0) {
			//print_trapframe(tf);
			if (pls_read(current) == NULL) {
				panic("handle pgfault failed. \n");
			} else {
				if (trap_in_kernel(tf)) {
					panic
					    ("handle pgfault failed in kernel mode.\n");
				}
				kprintf("killed by kernel.\n");
				do_exit(-E_KILLED);
			}
		}
		break;
	case T_TLB_PERMISSION_EXE:	//TLB permission violation (execute)
		panic
		    ("TLB permission violation(execute)! pteaddr=0x%x  badaddr=0x%x\n",
		     pteaddr, badaddr);
		break;
	case T_TLB_PERMISSION_READ:	//TLB permission violation (read)
		panic
		    ("TLB permission violation(read)! pteaddr=0x%x  badaddr=0x%x\n",
		     pteaddr, badaddr);
		break;
	case T_TLB_PERMISSION_WRITE:	//TLB permission violation (write)
		//kprintf("TLB permission violation(write)! pteaddr=0x%x  badaddr=0x%x\n", pteaddr, badaddr);
		if (tlb_miss_handler(badaddr, 1) != 0) {
			//print_trapframe(tf);
			if (pls_read(current) == NULL) {
				panic("handle pgfault failed.\n");
			} else {
				if (trap_in_kernel(tf)) {
					panic
					    ("handle pgfault failed in kernel mode. %\n");
				}
				kprintf("killed by kernel.\n");
				do_exit(-E_KILLED);
			}
		}
		//print_trapframe(tf);
		//panic("TLB permission violation(write)! pteaddr=0x%x  badaddr=0x%x\n", pteaddr, badaddr);
		break;
	default:
		panic("unknown exception! cause=%d. System is stopped.\n",
		      cause);
		break;
	}
}

static void trap_dispatch(struct trapframe *tf)
{
	if (irq_dispatch(tf) == 0) {
		//if there is no irq, process exception.
		exception_dispatch(tf);
	}
}

void trap(struct trapframe *tf)
{
	int status;
	NIOS2_READ_STATUS(status);
	NIOS2_WRITE_STATUS(status & (~NIOS2_STATUS_EH_MSK));
	// dispatch based on what type of trap occurred
	// used for previous projects
	if (current == NULL) {
		trap_dispatch(tf);
	} else {
		struct trapframe *otf = pls_read(current)->tf;
		pls_read(current)->tf = tf;

		bool in_kernel = trap_in_kernel(tf);

		trap_dispatch(tf);

		pls_read(current)->tf = otf;
		if (!in_kernel) {
			may_killed();
			if (pls_read(current)->need_resched) {
				schedule();
			}
		}

	}
	forkrets(tf);
}

int ucore_in_interrupt()
{
	//panic("ucore_in_interrupt()");
	return 0;
}
