#ifndef __ARCH_UM_INCLUDE_LINUX_SIGNAL_H__
#define __ARCH_UM_INCLUDE_LINUX_SIGNAL_H__

/* signal code */
#ifndef SIGKILL
#define NSIG		32	/* Number of SIGnals */
#define SIGHUP		 1
#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGTRAP		 5
#define SIGABRT		 6
#define SIGIOT		 6
#define SIGBUS		 7
#define SIGFPE		 8
#define SIGKILL		 9
#define SIGUSR1		10
#define SIGSEGV		11
#define SIGUSR2		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGSTKFLT	16
#define SIGCHLD		17
#define SIGCONT		18
#define SIGSTOP		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU		22
#define SIGURG		23
#define SIGXCPU		24
#define SIGXFSZ		25
#define SIGVTALRM	26
#define SIGPROF		27
#define SIGWINCH	28
#define SIGIO		29
#define SIGPOLL		SIGIO
#define SIGPWR		30
#define SIGSYS		31
#define	SIGUNUSED	31
#define SIGRTMIN	32
#define SIGRTMAX	_NSIG
#endif /* SIGKILL */

/* Bits in `sa_flags'.  */
#define	SA_NOCLDSTOP  1		/* Don't send SIGCHLD when children stop.  */
#define SA_NOCLDWAIT  2		/* Don't create zombie on child death.  */
#define SA_SIGINFO    4		/* Invoke signal-catching function with
				   three arguments instead of one.  */
#define SA_ONSTACK   0x08000000	/* Use signal stack by using `sa_restorer'. */
#define SA_RESTART   0x10000000	/* Restart syscall on signal return.  */
#define SA_NODEFER   0x40000000	/* Don't automatically block the signal when
				   its handler is being executed.  */
#define SA_RESETHAND 0x80000000	/* Reset to SIG_DFL on entry to handler.  */

/* Values for the HOW argument to `sigprocmask'.  */
#define	SIG_BLOCK     0		/* Block signals.  */
#define	SIG_UNBLOCK   1		/* Unblock signals.  */
#define	SIG_SETMASK   2		/* Set the set of blocked signals.  */

#ifndef __ASSEMBLER__

#include <string.h>		/* for memset */
#include <types.h>

#define __SI_MAX_SIZE     128
#define __SI_PAD_SIZE     ((__SI_MAX_SIZE / sizeof (int)) - 3)
typedef union sigval {
	int sival_int;
	void *sival_ptr;
} sigval_t;
typedef struct siginfo {
	int si_signo;		/* Signal number.  */
	int si_errno;		/* If non-zero, an errno value associated with
				   this signal, as defined in <errno.h>.  */
	int si_code;		/* Signal code.  */

	union {
		int _pad[__SI_PAD_SIZE];

		/* kill().  */
		struct {
			int32_t si_pid;	/* Sending process ID.  */
			int32_t si_uid;	/* Real user ID of sending process.  */
		} _kill;

		/* POSIX.1b timers.  */
		struct {
			int si_tid;	/* Timer ID.  */
			int si_overrun;	/* Overrun count.  */
			sigval_t si_sigval;	/* Signal value.  */
		} _timer;

		/* POSIX.1b signals.  */
		struct {
			int32_t si_pid;	/* Sending process ID.  */
			int32_t si_uid;	/* Real user ID of sending process.  */
			sigval_t si_sigval;	/* Signal value.  */
		} _rt;

		/* SIGCHLD.  */
		struct {
			int32_t si_pid;	/* Which child.  */
			int32_t si_uid;	/* Real user ID of sending process.  */
			int si_status;	/* Exit value or signal.  */
			long int si_utime;
			long int si_stime;
		} _sigchld;

		/* SIGILL, SIGFPE, SIGSEGV, SIGBUS.  */
		struct {
			void *si_addr;	/* Faulting insn/memory ref.  */
			int _trapno;	/* TRAP # which caused the signal */
		} _sigfault;

		/* SIGPOLL.  */
		struct {
			long int si_band;	/* Band event for SIGPOLL.  */
			int si_fd;
		} _sigpoll;
	} _sifields;
} siginfo_t;

#define si_pid		_sifields._kill.si_pid
#define si_uid		_sifields._kill.si_uid
#define si_timerid	_sifields._timer.si_tid
#define si_overrun	_sifields._timer.si_overrun
#define si_status	_sifields._sigchld.si_status
#define si_utime	_sifields._sigchld.si_utime
#define si_stime	_sifields._sigchld.si_stime
#define si_value	_sifields._rt.si_sigval
#define si_int		_sifields._rt.si_sigval.sival_int
#define si_ptr		_sifields._rt.si_sigval.sival_ptr
#define si_addr		_sifields._sigfault.si_addr
#define si_trapno	_sifields._sigfault._trapno
#define si_band		_sifields._sigpoll.si_band
#define si_fd		_sifields._sigpoll.si_fd

/* `si_code' values for SIGSEGV signal.  */
enum {
	SEGV_MAPERR = 1,	/* Address not mapped to object.  */
#define SEGV_MAPERR	SEGV_MAPERR
	SEGV_ACCERR		/* Invalid permissions for mapped object.  */
#define SEGV_ACCERR	SEGV_ACCERR
};

#define _NSIG         1024
#define _NSIG_BPW     (8 * sizeof (unsigned long int))
#define _NSIG_WORDS	  (_NSIG / _NSIG_BPW)
typedef struct {
	unsigned long sig[_NSIG_WORDS];
} sigset_t;

typedef void (*__sighandler_t) (int);

#define SIG_ERR	((__sighandler_t) -1)	/* Error return.  */
#define SIG_DFL	((__sighandler_t) 0)	/* Default action.  */
#define SIG_IGN	((__sighandler_t) 1)	/* Ignore signal.  */

struct sigaction {
	union {
		__sighandler_t _sa_handler;
		void (*_sa_sigaction) (int, struct siginfo *, void *);
	} _u;
	sigset_t sa_mask;
	unsigned long sa_flags;
	void (*sa_restorer) (void);
};
#define sa_handler	_u._sa_handler
#define sa_sigaction	_u._sa_sigaction

typedef struct sigaltstack {
	void *ss_sp;
	uint32_t ss_flags;
	size_t ss_size;
} stack_t;

extern char *sig_names[];

int raise(int signum);
void sigemptyset(sigset_t * set);
void sigaddset(sigset_t * set, int _sig);
int sigismember(sigset_t * set, int _sig);

#endif /* __ASSEMBLER__ */

#endif /* !__ARCH_UM_INCLUDE_LINUX_SIGNAL_H__ */
