#include <trap.h>
#include <arch.h>
#include <proc.h>
#include <stdio.h>
#include <stub.h>
#include <memlayout.h>
#include <mmu.h>
#include <pmm.h>
#include <vmm.h>
#include <kio.h>

#define current pls_read(current)

/*
 * Signals that are OK to receive in the stub - we'll just continue.
 * SIGWINCH will happen when UML is inside a detached screen.
 */
#define STUB_SIG_MASK ((1 << SIGVTALRM) | (1 << SIGWINCH))

/* Signals that the stub will finish with - anything else is an error */
#define STUB_DONE_MASK (1 << SIGTRAP)

/**
 * Run the stub code. The main process will simply wait it done (ie. SIGTRAP is captured).
 * @param pid the process id on host system of the container process
 * @return 0 on success, or a negative (depends on the signal which makes it fail) otherwise
 */
int wait_stub_done(int pid)
{
	int n, status, err;

	struct um_pt_regs regs;

	while (1) {
		n = syscall3(__NR_waitpid, pid, (long)&status,
			     WUNTRACED | __WALL);
		if (WIFEXITED(status)) {
			kprintf("stub: status = 0x%x\n", status);
			kprintf("stub: err = %d\n", err);
			kprintf("stub: eip = 0x%x\n", regs.regs.eip);
		}

		if ((n < 0) || !WIFSTOPPED(status))
			goto bad_wait;

		if (WSTOPSIG(status) == SIGSEGV) {	/* Possibly first read or write */
			/* create a new stub frame for page fault handling */
			stub_push_frame(current->arch.host->stub_stack);
			if ((err =
			     syscall4(__NR_ptrace, PTRACE_GETREGS, pid, 0,
				      (long)&(regs.regs))) < 0) {
				return -3;
			}

			if (user_segv(pid, &regs) < 0)
				goto bad_wait;

			stub_pop_frame(current->arch.host->stub_stack);
			if ((err =
			     syscall4(__NR_ptrace, PTRACE_SETREGS, pid, 0,
				      (long)&(regs.regs))) < 0) {
				return -3;
			}
		} else if (((1 << WSTOPSIG(status)) & STUB_SIG_MASK) == 0)
			break;

		err = syscall4(__NR_ptrace, PTRACE_CONT, pid, 0, 0);
		if (err) {
			return -2;
		}
	}

	if (((1 << WSTOPSIG(status)) & STUB_DONE_MASK) != 0)
		return 0;

	return -WSTOPSIG(status);

bad_wait:
	return -1;
}

/**
 * Extract details of the SIGSEGV from the faultinfo.
 * Note: When writing to a clean page, there will be two signals raised.
 *       When the first signal arises, a 'read' operation will be extracted
 *           and the page will be made readable but not writable, so write to it will raise a SIGSEVG again.
 *       And when the second signal comes, the 'write' operation will be extracted
 *           and the page will be finally writabble.
 * @param pid the process id on host system of the container process
 * @param fi the faultinfo got with the signal
 * @return 0 on success, or a negative otherwise
 */
int get_faultinfo(int pid, struct faultinfo *fi)
{
	int err;

	siginfo_t siginfo;
	err = syscall4(__NR_ptrace, PTRACE_GETSIGINFO, pid, 0, (long)&siginfo);
	if (err != 0)
		return -4;

	fi->cr2 = (int)siginfo.si_addr;
	fi->trap_no = siginfo.si_trapno;
	pte_t *pte;
	switch (siginfo.si_code) {
	case SEGV_MAPERR:
		/* MAPERR indicates that no mapping is found in the container process */
		fi->error_code = 4;
		break;
	case SEGV_ACCERR:
		/* ACCERR indicates that read/write is not permitted though the map exists */
		pte = get_pte(current->mm->pgdir, fi->cr2, 0);
		if (*pte & PTE_A)
			/* If the page has already been read, it should be a writing which raises the signal */
			fi->error_code = 7;
		else
			fi->error_code = 5;
		break;
	}

	/* err = syscall4 (__NR_ptrace, PTRACE_CONT, pid, 0, SIGSEGV); */
	/* if (err != 0) */
	/*      return -1; */

	/* err = wait_stub_done (pid); */
	/* if (err < 0) */
	/*      return err - 10; */

	/* memcpy(fi, (void*)((int)current->arch.host->stub_stack + 0x800), sizeof(*fi)); */

	return 0;
}

/**
 * 'Cancel' a syscall by the user.
 *     Syscalls from the user are actually interpreted by the main process.
 *     However, 'ptrace' can detect a syscall only when it has been made or it is almost done.
 *     So, the first time a 'syscall' notification is got,
 *         the main process will make the container execute another syscall with almost no effects (getpid here)
 *         and let the container go on, what this function mainly does.
 *     And when 'getpid' is done successfully, this function returns
 *         and the main process continue its interpreting.
 * @param pid the process id on host system of the container
 * @param the copy of container's registers
 * @return 0 on success, or -1 otherwise
 */
int nullify_syscall(int pid, struct um_pt_regs *regs)
{
	int err, status;

	/* Write __NR_getpid to orig_eax, so a 'getpid' of no harm will be executed instead */
	err = syscall4(__NR_ptrace, PTRACE_POKEUSR, pid, 44, __NR_getpid);
	if (err < 0) {
		kprintf("nullify_syscall: cannot nullify\n");
		return -1;
	}

	err = syscall4(__NR_ptrace, PTRACE_SYSCALL, pid, 0, 0);
	if (err < 0) {
		kprintf("nullify_syscall: cannot continue syscall\n");
		return -1;
	}

	err = syscall3(__NR_waitpid, pid, (long)&status, WUNTRACED | __WALL);
	if (err < 0 || !WIFSTOPPED(status)
	    || (WSTOPSIG(status) != (SIGTRAP | 0x80))) {
		kprintf("nullify_syscall: wait error\n");
		kprintf("\terr = %d\n", err);
		kprintf("\tis stopped = %d\n", WIFSTOPPED(status));
		kprintf("\tstop sig = %d\n", WSTOPSIG(status));
		return -1;
	}

	return 0;
}
