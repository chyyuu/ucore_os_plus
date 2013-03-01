#include <arch.h>
#include <proc.h>
#include <types.h>
#include <mmu.h>
#include <stdio.h>
#include <memlayout.h>
#include <pmm.h>
#include <vmm.h>
#include <stub.h>
#include <sched.h>
#include <error.h>
#include <host_signal.h>
#include <host_syscall.h>
#include <syscall.h>
#include <kio.h>
#include <umclock.h>

/**
 * Dump all registers from the copy in PCB.
 *     Used when someone is killed by the kernel.
 * @param pt_regs the copy of register contents and process status.
 */
void dump_regs(struct um_pt_regs *pt_regs)
{
	struct user_regs_struct *regs = &(pt_regs->regs);
	kprintf("syscall = %d\n"
		"is_user = %d\n"
		"cr2 = 0x%x\n",
		pt_regs->syscall, pt_regs->is_user, pt_regs->faultinfo.cr2);
	kprintf("  eax\t0x%08x\n"
		"  oeax\t0x%08x\n"
		"  ebx\t0x%08x\n"
		"  ecx\t0x%08x\n"
		"  edx\t0x%08x\n"
		"  esi\t0x%08x\n"
		"  edi\t0x%08x\n"
		"  eip\t0x%08x\n"
		"  esp\t0x%08x\n"
		"  ebp\t0x%08x\n"
		"  cs\t----0x%04x\n"
		"  ds\t----0x%04x\n"
		"  es\t----0x%04x\n"
		"  fs\t----0x%04x\n"
		"  gs\t----0x%04x\n"
		"  ss\t----0x%04x\n",
		regs->eax, regs->orig_eax, regs->ebx, regs->ecx, regs->edx,
		regs->esi, regs->edi, regs->eip, regs->esp, regs->ebp,
		regs->xcs, regs->xds, regs->xes, regs->xfs, regs->xgs,
		regs->xss);
}

/**
 * Kill a container process.
 * @param pid the process id on host system of the container process
 * @param wait_done whether the main process should wait until the container is killed
 */
void kill_ptraced_process(int pid, int wait_done)
{
	syscall2(__NR_kill, pid, SIGKILL);
	syscall4(__NR_ptrace, PTRACE_KILL, pid, 0, 0);
	syscall4(__NR_ptrace, PTRACE_CONT, pid, 0, 0);	/* the container may be stopped at present */
	if (wait_done)
		syscall3(__NR_waitpid, pid, 0, __WALL);
}

/**
 * Create a new container process, making it use @stack and execute @fn with arguments @args.
 * @param fn the function that the child should execute
 * @param stack the stack for the child to use
 * @param args the arguments for the child
 * @return 0 on success, or -1 otherwise
 */
int clone(int (*fn) (void *), void *stack, void *args)
{
	int ret;

	ret = syscall0(__NR_fork);
	if (ret < 0) {		/* error */
		return ret;
	} else if (ret == 0) {	/* the child */
		asm("movl %%ecx, %%ebp;"
		    "movl %%edx, %%esp;"
		    "jmp kernel_thread_entry"::"b"(fn), "c"(args), "d"(stack));
	}

	return ret;		/* the parent */
}

/**
 * Entry of the child processes, doing all initializing work for a newly created container.
 * @param stub_stack the stub stack allocated for the container
 * @return (never returns for the host will make it run another segment of code after it stops)
 */
static int userspace_tramp(void *stub_stack)
{
	umclock_init();

	syscall4(__NR_ptrace, PTRACE_TRACEME, 0, 0, 0);

	/* Cleanup everything */
	syscall2(__NR_munmap, USERBASE, USERTOP - USERBASE);

	/* Map STUB_CODE and STUB_DATA */
	{
		struct mmap_arg_struct mmap_args = {
			.addr = STUB_CODE,
			.len = PGSIZE,
			.prot = PROT_READ | PROT_EXEC,
			.flags = MAP_SHARED | MAP_FIXED,
			.fd = ginfo->mem_fd,
			.offset = PADDR(&__syscall_stub_start)
		};
		int ret = syscall1(__NR_mmap, (long)&mmap_args);
		if (ret < 0)
			syscall1(__NR_exit, 1);
	}
	{
		struct mmap_arg_struct mmap_args = {
			.addr = STUB_DATA,
			.len = PGSIZE,
			.prot = PROT_READ | PROT_WRITE,
			.flags = MAP_SHARED | MAP_FIXED,
			.fd = ginfo->mem_fd,
			.offset = PADDR(stub_stack)
		};
		int ret = syscall1(__NR_mmap, (long)&mmap_args);
		if (ret < 0)
			syscall1(__NR_exit, 1);
	}

	/* set SIGSEGV 'handler' (just collect info needed for the tracer) */
	struct sigaction sa;
	unsigned long v =
	    STUB_CODE + (unsigned long)stub_segv_handler -
	    (unsigned long)&__syscall_stub_start;
	struct sigaltstack ss = {
		.ss_flags = 0,
		.ss_sp = (void *)(STUB_DATA),
		.ss_size = PGSIZE - sizeof(void *)
	};
	if (syscall2(__NR_sigaltstack, (long)&ss, 0) != 0)
		syscall1(__NR_exit, 1);
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_ONSTACK | SA_NODEFER;
	sa.sa_handler = (void *)v;
	sa.sa_restorer = NULL;
	if (syscall3(__NR_sigaction, SIGSEGV, (long)&sa, 0) < 0) {
		syscall1(__NR_exit, 1);
	}

	raise(SIGSTOP);

	return 0;
}

/**
 * Create a container process to simulate a userspace.
 * @param stub_stack the stub stack allocated for the container
 * @return process id of the container on success, or -1 otherwise
 */
int start_userspace(void *stub_stack)
{
	int pid, status, n;
	unsigned long sp;

	/* stub_stack is used as a temporary stack for the child */
	sp = (unsigned long)stub_stack + PGSIZE - sizeof(void *);
	pid = clone(userspace_tramp, (void *)sp, (void *)stub_stack);
	if (pid < 0) {
		kprintf("start userspace: clone failed with %d\n", pid);
		return -1;
	}

	/* wait until the child stops, ignoring timer interrupts */
	do {
		n = syscall3(__NR_waitpid, pid, (long)&status,
			     WUNTRACED | __WALL);
		if (n < 0) {
			kprintf("start userspace: wait failed\n");
			goto out_kill;
		}
	} while (WIFSTOPPED(status) && (WSTOPSIG(status) == SIGVTALRM));

	if (!WIFSTOPPED(status) || (WSTOPSIG(status) != SIGSTOP)) {
		kprintf("start userspace: expected SIGSTOP\n");
		goto out_kill;
	}
	if (syscall4
	    (__NR_ptrace, PTRACE_OLDSETOPTIONS, pid, 0,
	     PTRACE_O_TRACESYSGOOD) < 0) {
		kprintf("start userspace: cannot trace syscalls\n");
		goto out_kill;
	}

	/* Initialize stub stack */
	struct stub_stack *stack = (struct stub_stack *)stub_stack;
	stack->current_no = 0;
	stack->current_addr =
	    (uintptr_t) & (stack->frames) - (uintptr_t) stack + STUB_DATA;

	return pid;

out_kill:
	kill_ptraced_process(pid, 1);
	return -1;
}

/**
 * Call segv_handler and if it failed in kernel mode, panic the system.
 * @param regs the status stored
 * @return 0 on success, or -1 if failed for user process, or panic if failed in kernel
 */
static int do_segv(struct um_pt_regs *regs)
{
	if (segv_handler(SIGSEGV, regs) != 0) {
		if (!regs->is_user) {
			panic("handle pgfault failed in kernel mode. \n");
		}
		return -1;
	}
	return 0;
}

/**
 * Handle user SIGSEGV
 * @param pid the process id on host system of the container process
 * @param regs the storage of registers and status
 * @return 0 on sucess, or -1 otherwise
 */
int user_segv(int pid, struct um_pt_regs *regs)
{
	/* Extract detailed info */
	int err = get_faultinfo(pid, &(regs->faultinfo));
	if (err != 0) {
		kprintf("userspace: cannot get fault info. err = %d\n", err);
		return -1;
	}
	/* Try handling the page fault */
	if (do_segv(regs) < 0) {
		kprintf("Segmentation fault at 0x%x\n", regs->faultinfo.cr2);
		return -1;
	}
	return 0;
}

/**
 * The main loop for monitor threads.
 * @param regs register and status storage to be used (may be dummy)
 */
void userspace(struct um_pt_regs *regs)
{
	int err, status, pid = pls_read(current)->arch.host->host_pid;
	pls_read(current)->arch.forking = 0;
	regs->is_user = 1;

	while (1) {
		/* As a host process may be the container of multiple threads, we need to reread 'regs' */
		regs = &(pls_read(current)->arch.regs);

		err =
		    syscall4(__NR_ptrace, PTRACE_SETREGS, pid, 0,
			     (long)&(regs->regs));
		if (err != 0) {
			kprintf("userspace: cannot set regs: %d\n", err);
			goto exit;
		}
		//kprintf ("start at eip = 0x%x\n", regs->regs.eip);

		/* it seems that my fedora doesn't support PTRACE_SYSEMU (sigh...) */
		if (syscall4(__NR_ptrace, PTRACE_SYSCALL, pid, 0, 0) != 0) {
			kprintf("userspace: ptrace continue failed\n");
			goto exit;
		}

		err =
		    syscall3(__NR_waitpid, pid, (long)&status,
			     WUNTRACED | __WALL);

		if (err < 0) {
			kprintf("userspace: wait failed\n");
			goto exit;
		}
		if (syscall4
		    (__NR_ptrace, PTRACE_GETREGS, pid, 0,
		     (long)&(regs->regs)) != 0) {
			kprintf("userspace: get regs failed\n");
			goto exit;
		}
		//kprintf ("stop at eip = 0x%x\n", regs->regs.eip);

		regs->syscall = -1;
		if (WIFSTOPPED(status)) {
			int sig = WSTOPSIG(status);
			switch (sig) {
			case SIGSEGV:
				if (user_segv(pid, regs) < 0)
					goto exit;
				break;
			case SIGTRAP + 0x80:	/* system call */
				if (nullify_syscall(pid, regs) < 0)
					goto exit;
				regs->syscall = regs->regs.orig_eax;
				regs->regs.eax = syscall(regs);
				break;
			case SIGVTALRM:
				vtimer_handler(SIGVTALRM, regs);
				break;
			case SIGIO:
				io_handler(SIGIO, regs);
				break;
			case SIGFPE:
				kprintf("Floating Point Exception\n");
				goto exit;
			default:
				kprintf("userspace: unknown signal %d\n", sig);
				goto exit;
			}
		}

		if (pls_read(current)->flags & PF_EXITING) {
			do_exit_thread(-E_KILLED);
		}
		if (pls_read(current)->need_resched) {
			schedule();
		}
	}

exit:
	kprintf("process %d killed by kernel.\n", pls_read(current)->pid);
	dump_regs(regs);
	int stack = regs->regs.esp;
	pte_t *ptep = get_pte(pls_read(current)->mm->pgdir, stack, 0);
	uint8_t *addr = (uint8_t *) (PTE_ADDR(*ptep) + PGOFF(stack) + KERNBASE);
	assert(ptep != NULL);
	int i, j;
	for (i = 0; i < 2; i++) {
		kprintf("0x%08x\t", stack);
		for (j = 0; j < 16; j++)
			kprintf("%02x ", addr[j]);
		kprintf("\n");
		stack += 16;
		addr += 16;
	}
	do_exit_thread(err);
}
