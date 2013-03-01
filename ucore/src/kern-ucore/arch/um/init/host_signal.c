#include <host_signal.h>

#include <arch.h>
#include <assert.h>
#include <stdio.h>
#include <pmm.h>		/* tlb_update */
#include <vmm.h>		/* do_pgfault */
#include <clock.h>		/* ticks */
#include <sched.h>		/* run_timer_list */
#include <console.h>
#include <kio.h>
#include <proc.h>

#define current pls_read(current)
#define idleproc pls_read(idleproc)

int segv_handler(int, struct um_pt_regs *);
int io_handler(int, struct um_pt_regs *);
int vtimer_handler(int, struct um_pt_regs *);

int (*sig_handlers[NSIG]) (int sig, struct um_pt_regs * regs) = {
[SIGSEGV] = segv_handler,
	    [SIGIO] = io_handler,[SIGVTALRM] = vtimer_handler};

/**
 * make a copy of the signal context
 * @param regs container for the context
 * @param context given by the host
 */
static void copy_sc(struct um_pt_regs *regs, struct sigcontext *from)
{
	regs->regs.eax = from->eax;
	regs->regs.ebx = from->ebx;
	regs->regs.ecx = from->ecx;
	regs->regs.edx = from->edx;
	regs->regs.eip = from->eip;

	regs->faultinfo.cr2 = from->cr2;
	/* 'handle_signal' handles signals raised by the tracer itself */
	regs->faultinfo.error_code = from->err & ~(0x4);
	regs->faultinfo.trap_no = from->trapno;
}

/**
 * call the corresponding signal handler according to the sig number
 * @param sig signal serial number
 * @param sc context of the signal
 */
void handle_signal(int sig, struct sigcontext *sc)
{
	struct um_pt_regs r;
	r.is_user = 0;
	if (sig == SIGSEGV)
		copy_sc(&r, sc);
	(*sig_handlers[sig]) (sig, &r);
}

/**
 * The entry of all signal handlers
 * @param sig signal serial number
 */
void hard_handler(int sig)
{
	handle_signal(sig, (struct sigcontext *)(&sig + 1));
}

/**
 * Register signal handler
 * @param sig signal serial number
 * @param handler the signal handler specified
 * @param flags sigaction-related flags
 */
void set_handler(int sig, void (*handler) (int), int flags)
{
	struct sigaction action;
	if (handler == NULL)
		action.sa_handler = hard_handler;
	else
		action.sa_handler = handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = flags;
	action.sa_restorer = NULL;
	if (syscall3(__NR_sigaction, sig, (long)&action, 0) < 0)
		panic("sigaction failed\n");
}

/**
 * Check whether traps work as expected
 */
void check_trap(void)
{
	/* SIGSEGV */
	*(int *)(0xA0000000) = 15;

	kprintf("check_trap() succeeded.\n");
}

/**
 * Initialize basic signal handlers
 */
void host_signal_init(void)
{
	set_handler(SIGSEGV, NULL, 0);
	set_handler(SIGVTALRM, NULL, 0);
	set_handler(SIGIO, NULL, 0);

	set_handler(SIGQUIT, host_exit, 0);
}

/**************************************************
 * Specific signal handlers
 **************************************************/

/**
 * an error has occured.
 */
static int bad_segv(int loc)
{
	if (current == NULL || !current->arch.regs.is_user)
		panic("Kernel segmentation fault at %d\n", loc);
	return -1;
}

static inline void print_pgfault(int addr, int error_code)
{
	/* error_code:
	 * bit 0 == 0 means no page found, 1 means protection fault
	 * bit 1 == 0 means read, 1 means write
	 * bit 2 == 0 means kernel, 1 means user
	 * */
	return;

	kprintf("page fault at 0x%08x: %c/%c [%s].\n", addr,
		(error_code & 4) ? 'U' : 'K',
		(error_code & 2) ? 'W' : 'R',
		(error_code & 1) ? "protection fault" : "no page found");
}

/**
 * mark a page as accessed or dirty and flush tlb
 * @param mm mm_struct from which to get pgdir
 * @param is_write whether writing is performed
 * @param addr the logical address to be accessed
 */
int do_pgmark(struct mm_struct *mm, uint32_t error_code, uintptr_t addr)
{
	uint32_t is_write = (error_code & FAULT_W) ? 1 : 0;
	struct vma_struct *vma = find_vma(mm, addr);
	if (vma == NULL || vma->vm_start > addr) {
		kprintf("invalid address: 0x%x\n", addr);
		goto failed;
	}

	pte_t *pte = get_pte(mm->pgdir, addr, 0);
	if (pte == 0) {
		kprintf("pte not exists\n");
		goto failed;
	}
	if (is_write && Get_PTE_W(pte) == 0) {
		/* Maybe it's a shared memory.
		 * call do_pgfault and check the return val. */
		print_pgfault(addr, error_code);
		if ((do_pgfault(mm, error_code, addr)) != 0) {
			kprintf("shared memory handle failed\n");
			goto failed;
		}
	}

	Set_PTE_A(pte);
	if (is_write) {
		Set_PTE_D(pte);
	}

	tlb_update(mm->pgdir, addr);

	return 0;

failed:
	return bad_segv(-1);
}

/**
 * SIGSEGV handler
 *     give an error message first
 * @param sig signal serial number (of no use?)
 * @param regs register contexts of the fault
 */
int segv_handler(int sig, struct um_pt_regs *regs)
{
	int err;

	if (regs->faultinfo.cr2 < 0x10000)
		return bad_segv(regs->faultinfo.cr2);

	extern struct mm_struct *check_mm_struct;
	struct mm_struct *mm;
	if (check_mm_struct != NULL) {	/* for mm checks only */
		assert(current == idleproc);
		mm = check_mm_struct;
	} else {
		if (current == NULL) {
			return bad_segv(-2);
		}
		mm = current->mm;
	}

	pte_t *pte = get_pte(mm->pgdir, regs->faultinfo.cr2, 0);
	if (pte == 0 || Get_PTE_P(pte) == 0) {	/* Page not present. Normal page fault */
		print_pgfault(regs->faultinfo.cr2, regs->faultinfo.error_code);
		if ((err =
		     do_pgfault(mm, regs->faultinfo.error_code,
				regs->faultinfo.cr2)) != 0)
			return bad_segv(regs->faultinfo.cr2);
	} else {
		print_pgfault(regs->faultinfo.cr2, regs->faultinfo.error_code);
		int ret = do_pgmark(mm, regs->faultinfo.error_code,
				    regs->faultinfo.cr2);
		/* uint32_t* ebp; */
		/* asm volatile ("movl %%ebp, %0" : "=r" (ebp)); */
		/* int i; */
		/* for (i = 0; i < 5; i++) */
		/*      kprintf ("[%d] 0x%x\n", i*4, ebp[i]); */
		/* kprintf ("=====\n"); */
		return ret;
	}
	return 0;
}

/**
 * SIGIO handler
 *     silently ignored
 * @param sig signal serial number
 * @param regs register contexts of the fault
 */
int io_handler(int sig, struct um_pt_regs *regs)
{
	char c = cons_getc();

	extern void dev_stdin_write(char c);
	dev_stdin_write(c);

	return 0;
}

int vtimer_handler(int sig, struct um_pt_regs *regs)
{
	ticks++;
	assert(current != NULL);
	run_timer_list();
	return 0;
}
