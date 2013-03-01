#include <linux/signal.h>

#include <arch.h>

char *sig_names[] = {
	"",			/* 0 */
	"",
	"SIGINT",
	"",
	"",
	"SIGTRAP",		/* 5 */
	"",
	"",
	"",
	"",
	"SIGUSR1",		/* 10 */
	"SIGSEGV",
	"",
	"",
	"",
	"",			/* 15 */
	"",
	"SIGCHLD",
	"",
	"",
	"",			/* 20 */
	"",
	"",
	"",
	"",
	"",			/* 25 */
	"",
	"",
	"",
	"SIGIO/SIGPOLL",
	"",			/* 30 */
	"",
	""
};

/**
 * raise a signal to the calling process
 * @param signum 
 */
int raise(int signum)
{
	syscall2(__NR_kill, syscall0(__NR_getpid), signum);
	return 0;
}

/**
 * fill a signal set with zero
 * @param set the signal set to fill
 */
inline void sigemptyset(sigset_t * set)
{
	switch (_NSIG_WORDS) {
	default:
		memset(set, 0, sizeof(sigset_t));
		break;
	case 2:
		set->sig[1] = 0;
	case 1:
		set->sig[0] = 0;
		break;
	}
}

/**
 * add the specified signal to the set
 * @param set signal set where the specified signal is to be put in
 * @param _sig the specified signal
 */
inline void sigaddset(sigset_t * set, int _sig)
{
	unsigned long sig = _sig - 1;
	if (_NSIG_WORDS == 1)
		set->sig[0] |= 1UL << sig;
	else
		set->sig[sig / _NSIG_BPW] |= 1UL << (sig % _NSIG_BPW);
}

/**
 * check whether @_sig is in @set
 * @param set a set of signals to be checked
 * @param _sig the specified signal
 * @return 1 if @_sig is in @set and 0 otherwise
 */
inline int sigismember(sigset_t * set, int _sig)
{
	unsigned long sig = _sig - 1;
	if (_NSIG_WORDS == 1)
		return 1 & (set->sig[0] >> sig);
	else
		return 1 & (set->sig[sig / _NSIG_BPW] >> (sig % _NSIG_BPW));
}
