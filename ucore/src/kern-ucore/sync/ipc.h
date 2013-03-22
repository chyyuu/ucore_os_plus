#ifndef __KERN_SYNC_IPC_H__
#define __KERN_SYNC_IPC_H__

#include <clock.h>
#include <sync.h>
#include <proc.h>
#include <sched.h>
#include <error.h>

static inline void ipc_add_timer(timer_t * timer)
{
	if (timer != NULL) {
		add_timer(timer);
	}
}

static inline void ipc_del_timer(timer_t * timer)
{
	if (timer != NULL) {
		del_timer(timer);
	}
}

static inline timer_t *ipc_timer_init(unsigned int timeout,
				      unsigned long *saved_ticks,
				      timer_t * timer)
{
	if (timeout != 0) {
		*saved_ticks = ticks;
		return timer_init(timer, current, timeout);
	}
	return NULL;
}

static inline int
ipc_check_timeout(unsigned int timeout, unsigned long saved_ticks)
{
	if (timeout != 0) {
		unsigned long delt = (unsigned long)(ticks - saved_ticks);
		if (delt >= timeout) {
			return -E_TIMEOUT;
		}
	}
	return -1;
}

#endif /* !__KERN_SYNC_IPC_H__ */
