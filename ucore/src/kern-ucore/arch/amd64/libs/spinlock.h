#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__
#include <arch.h>
#include <atomic.h>
#include <assert.h>

typedef struct spinlock_s {
	volatile unsigned int lock;
} spinlock_s;

typedef spinlock_s *spinlock_t;

#define spinlock_init(x) do { (x)->lock = 0; } while (0)


static inline void spinlock_acquire(spinlock_t lock)
{
	while(!atomic_compare_and_swap(&lock->lock, 0, 1))
		nop_pause();
}

static inline int spinlock_acquire_try(spinlock_t lock)
{
	return atomic_compare_and_swap(&lock->lock, 0, 1);
}

static inline void spinlock_release(spinlock_t lock)
{
	assert(lock->lock != 0);
	lock->lock = 0;
}

#endif
