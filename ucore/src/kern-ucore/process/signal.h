#ifndef _HERN_PROCESS_SIGNAL_H_
#define _HERN_PROCESS_SIGNAL_H_

#include <atomic.h>
#include <list.h>
#include <signum.h>
#include <trap.h>
#include <types.h>
#include <sem.h>

#define sigset_initwith(set, init)	\
	((set) = (init))

#define sigset_add(set, nsig)	\
	((set) |= 1ull << ((nsig) - 1))

#define sigset_del(set, nsig)	\
	((set) &= ~(1ull << ((nsig)- 1)))

#define sigset_addmask(set, mask)	\
	((set) |= (uint64_t)(mask))

#define sigset_delmask(set, mask)	\
	((set) &= ~(uint64_t)(mask))

#define sigismember(set, nsig)	\
	(1 & ((set) >> ((nsig) - 1)))

#define sigmask(nsig)	\
	(1ull << ((nsig) - 1))

struct sigpending {
	list_entry_t list;
	sigset_t signal;
};

struct sigqueue {
	list_entry_t list;
	semaphore_t *sem;
	uint32_t flags;
	struct siginfo_t info;
};

#define le2sigqueue(le)	\
	to_struct((le), struct sigqueue, list)

struct proc_struct;

struct signal_struct {
	atomic_t count;
	struct proc_struct *curr_target;
	struct sigpending shared_pending;
	int exit_code;
};

struct sighand_struct {
	atomic_t count;
	struct sigaction action[64];
	semaphore_t sig_sem;
};

struct proc_signal {
	struct signal_struct *signal;
	struct sighand_struct *sighand;
	sigset_t blocked;
	sigset_t rt_blocked;
	struct sigpending pending;
	uintptr_t sas_ss_sp;
	size_t sas_ss_size;
};

static inline int signal_count(struct signal_struct *sig)
{
	return atomic_read(&(sig->count));
}

static inline void set_signal_count(struct signal_struct *sig, int val)
{
	atomic_set(&(sig->count), val);
}

static inline int signal_count_inc(struct signal_struct *sig)
{
	return atomic_add_return(&(sig->count), 1);
}

static inline int signal_count_dec(struct signal_struct *sig)
{
	return atomic_sub_return(&(sig->count), 1);
}

static inline int sighand_count(struct sighand_struct *sh)
{
	return atomic_read(&(sh->count));
}

static inline void set_sighand_count(struct sighand_struct *sh, int val)
{
	atomic_set(&(sh->count), val);
}

static inline int sighand_count_inc(struct sighand_struct *sh)
{
	return atomic_add_return(&(sh->count), 1);
}

static inline int sighand_count_dec(struct sighand_struct *sh)
{
	return atomic_sub_return(&(sh->count), 1);
}

struct signal_struct *signal_create(void);
void signal_destroy(struct signal_struct *sig);

struct sighand_struct *sighand_create(void);
void sighand_destroy(struct sighand_struct *sh);

void lock_sig(struct sighand_struct *sh);
void unlock_sig(struct sighand_struct *sh);

int do_sigaction(int sign, const struct sigaction *act, struct sigaction *old);
int do_sigpending(sigset_t * set);
int do_sigprocmask(int how, const sigset_t * set, sigset_t * old);
int do_sigsuspend(sigset_t __user * pmask);
int do_sigreturn(void);
int do_sigaltstack(const stack_t * ss, stack_t * old);

int do_sigtkill(int pid, int sign);
int do_sigkill(int pid, int sign);

int do_sigwaitinfo(const sigset_t * set, struct siginfo_t *info);

int raise_signal(struct proc_struct *proc, int sign, bool group);

int do_signal(struct trapframe *tf, sigset_t * old);

void sig_recalc_pending(struct proc_struct *proc);

int __sig_setup_frame(int sign, struct sigaction *act, sigset_t oldset,
		      struct trapframe *tf);

#endif // _HERN_PROCESS_SIGNAL_H_
