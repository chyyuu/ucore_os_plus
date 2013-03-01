#ifndef __KERN_NIOS2_SYNC_SYNC_H__
#define __KERN_NIOS2_SYNC_SYNC_H__

//#include <mmu.h>
#include <assert.h>
//#include <atomic.h>
//#include <sched.h>
#include <nios2.h>

static inline int __intr_save(void)
{
	int context;
	NIOS2_READ_STATUS(context);
	NIOS2_WRITE_STATUS(context & ~NIOS2_STATUS_PIE_MSK);
	return context;
}

static inline void __intr_restore(int flag)
{
	NIOS2_WRITE_STATUS(flag);
}

#define local_intr_save(x)      do { x = __intr_save(); } while (0)
#define local_intr_restore(x)   __intr_restore(x);

#endif /* !__KERN_NIOS2_SYNC_SYNC_H__ */
