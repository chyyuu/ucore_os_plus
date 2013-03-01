#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include <atom.h>

typedef struct spinlock_s {
	volatile unsigned int lock;
} spinlock_s;

typedef spinlock_s *spinlock_t;

#define spinlock_init(x) do { (x)->lock = 0; } while (0)

static inline void spinlock_acquire(spinlock_t lock)
{
	while (xchg32(&lock->lock, 1) == 1) ;
}

static inline int spinlock_acquire_try(spinlock_t lock)
{
	return !xchg32(&lock->lock, 1);
}

static inline void spinlock_release(spinlock_t lock)
{
	lock->lock = 0;
}

#endif
