#ifndef __USER_LIBS_LOCK_H__
#define __USER_LIBS_LOCK_H__

#include <defs.h>
#include <ulib.h>

#define INIT_LOCK           {0}

typedef volatile bool lock_t;

static __always_inline bool test_and_set_bit(int nr, volatile uint32_t * addr)
{
	//TODO
	unsigned char c = 0;
	if (*addr & (1 << nr))
		c = 1;
	*addr |= (1 << nr);
	return c != 0;
}

static __always_inline bool test_and_clear_bit(int nr, volatile uint32_t * addr)
{
	//TODO
	unsigned char c = 0;
	if (*addr & (1 << nr))
		c = 1;
	*addr &= ~(1 << nr);
	return c != 0;
}

static inline void lock_init(lock_t * l)
{
	*l = 0;
}

static inline bool try_lock(lock_t * l)
{
	return test_and_set_bit(0, l);
}

static inline void lock(lock_t * l)
{
	if (try_lock(l)) {
		int step = 0;
		do {
			yield();
			if (++step == 100) {
				step = 0;
				sleep(10);
			}
		} while (try_lock(l));
	}
}

static inline void unlock(lock_t * l)
{
	test_and_clear_bit(0, l);
}

#endif /* !__USER_LIBS_LOCK_H__ */
