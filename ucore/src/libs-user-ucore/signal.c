#include <syscall.h>
#include "signal.h"

sighandler_t signal(int sign, sighandler_t handler)
{
	struct sigaction act = { handler, NULL, 1 << (sign - 1), 0 };
	sys_linux_sigaction(sign, &act, NULL);
	return handler;
}

int tkill(int pid, int sign)
{
	return sys_linux_tkill(pid, sign);
}

int kill_bionic(int pid, int sign)
{
	return sys_linux_kill(pid, sign);
}

int sigprocmask(int how, const sigset_t * set, sigset_t * old)
{
	return sys_linux_sigprocmask(how, set, old);
}

int sigsuspend(uint32_t mask)
{
	return sys_linux_sigsuspend(0, 0, mask);
}

#if 0
int set_shellrun_pid()
{
	return sys_set_shellrun_pid();
}
#endif
