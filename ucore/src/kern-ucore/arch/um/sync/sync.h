#ifndef __ARCH_UM_INCLUDE_SYNC_H__
#define __ARCH_UM_INCLUDE_SYNC_H__

#include <intr.h>
#include <arch.h>
#include <assert.h>
#include <atomic.h>

static inline bool __intr_save(void)
{
	sigset_t current;
	syscall3(__NR_sigprocmask, SIG_SETMASK, 0, (long)&current);
	if (sigismember(&current, SIGIO)) {
		intr_disable();
		return 1;
	}
	return 0;
}

static inline void __intr_restore(bool flag)
{
	if (flag) {
		intr_enable();
	}
}

#define local_intr_save(x)      do { x = __intr_save(); } while (0)
#define local_intr_restore(x)   __intr_restore(x);

void sync_init(void);

#endif /* !__ARCH_UM_INCLUDE_SYNC_H__ */
