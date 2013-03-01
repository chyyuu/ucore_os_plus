#ifndef __USER_LIBS_LOCK_H__
#define __USER_LIBS_LOCK_H__

#include <types.h>
#include <atomic.h>
#include <ulib.h>

#define INIT_LOCK           {0}

typedef volatile bool lock_t;

static inline void lock_init(lock_t * l)
{
	*l = 0;
}

static inline bool try_lock(lock_t * l)
{
#ifdef NO_LOCK
	return 0;
#else
	return test_and_set_bit(0, l);
#endif
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
#ifndef NO_LOCK
	test_and_clear_bit(0, l);
#endif
}

#endif /* !__USER_LIBS_LOCK_H__ */
