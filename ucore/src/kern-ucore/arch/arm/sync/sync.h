#ifndef __KERN_SYNC_SYNC_H__
#define __KERN_SYNC_SYNC_H__

#include <arm.h>
#include <intr.h>
#include <mmu.h>

static inline bool __intr_save(void)
{
	if (read_psrflags() & PSR_I) {
		return 0;
	}
	intr_disable();
	return 1;
}

static inline void __intr_restore(bool flag)
{
	if (flag) {
		intr_enable();
	}
}

#define local_intr_save(x)      do { x = __intr_save(); } while (0)
#define local_intr_restore(x)   __intr_restore(x);

/* note: lock functions have been moved to atomic.h */

#endif /* !__KERN_SYNC_SYNC_H__ */
