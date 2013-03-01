#ifndef __KERN_SYNC_LOCK_H__
#define __KERN_SYNC_LOCK_H__

#include <mmu.h>
#include <assert.h>
#include <atomic.h>
#include <sched.h>

typedef volatile bool lock_t;

static inline void lock_init(lock_t * lock)
{
	*lock = 0;
}

static inline bool try_lock(lock_t * lock)
{
	return !test_and_set_bit(0, (uint32_t *) lock);
}

static inline void lock(lock_t * lock)
{
	//cprintf("lock before: *lock=%d\n", *lock);
	while (!try_lock(lock)) {
		schedule();
	}
	//cprintf("lock after: *lock=%d\n", *lock);
}

static inline void unlock(lock_t * lock)
{
	if (!test_and_clear_bit(0, (uint32_t *) lock)) {
		panic("Unlock failed.\n");
	}
}

#endif /* !__KERN_SYNC_LOCK_H__ */
