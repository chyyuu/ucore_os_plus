#include <intr.h>
#include <arch.h>
#include <assert.h>
#include <stdio.h>
#include <kio.h>

static sigset_t disabled;

/**
 * Enable signal-simulated irqs and check whether we have any pending 'interrupts'.
 * TODO:
 *    signals that are pending will be fired automatically. what's wrong?
 */
void intr_enable(void)
{
	syscall3(__NR_sigprocmask, SIG_UNBLOCK, (long)&disabled, 0);

	/* static sigset_t pending; */
	/* syscall1 (__NR_sigpending, (long)&pending); */
	/* if (sigismember (&pending, SIGIO)) */
	/*      raise (SIGIO); */
}

/**
 * Disable signal-simulated irqs.
 *     simulated irqs: SIGIO
 */
void intr_disable(void)
{
	syscall3(__NR_sigprocmask, SIG_BLOCK, (long)&disabled, 0);
}

static int sigcount = 0;

/**
 * Temporary handler of SIGIO for check.
 * @param sig should be SIGIO
 */
void intr_check_handler(int sig)
{
	assert(sig == SIGIO);
	sigcount++;
}

/**
 * Check whether intr enable/disable works as expected
 */
void intr_check(void)
{
	//syscall2 (__NR_sys_signal, SIGIO, (long)intr_check_handler);
	struct sigaction sigact, defaultact;
	sigact.sa_handler = intr_check_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	syscall3(__NR_sigaction, SIGIO, (long)&sigact, (long)&defaultact);

	assert(sigcount == 0);

	raise(SIGIO);
	assert(sigcount == 1);
	raise(SIGIO);
	assert(sigcount == 2);

	intr_disable();
	raise(SIGIO);
	assert(sigcount == 2);
	raise(SIGIO);
	assert(sigcount == 2);

	intr_enable();
	assert(sigcount == 3);
	raise(SIGIO);
	assert(sigcount == 4);

	syscall3(__NR_sigaction, SIGIO, (long)&defaultact, 0);

	kprintf("intr_check() succeeded.\n");
}

/**
 * Initialize the signal sets used and check whether it works.
 *     should be called in device_init_common.
 */
void intr_init(void)
{
	sigemptyset(&disabled);
	sigaddset(&disabled, SIGIO);

	intr_check();
}
