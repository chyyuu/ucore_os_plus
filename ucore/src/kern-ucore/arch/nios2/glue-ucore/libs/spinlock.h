#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

typedef struct spinlock_s {
	volatile unsigned int lock;
} spinlock_s;

typedef spinlock_s *spinlock_t;

#define spinlock_init(x) do { (x)->lock = 0; } while (0)

static inline void spinlock_acquire(spinlock_t lock)
{
}

static inline int spinlock_acquire_try(spinlock_t lock)
{
	return 1;
}

static inline void spinlock_release(spinlock_t lock)
{
}

#endif
