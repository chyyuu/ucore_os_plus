#ifndef __ARCH_UM_DRIVERS_CLOCK_H__
#define __ARCH_UM_DRIVERS_CLOCK_H__

#include <types.h>

#define DEFAULT_TV_SEC    0
#define DEFAULT_TV_USEC   5000

#define TICK_NUM 30

extern volatile size_t ticks;

int umclock_init(void);
int umclock_set_interval(long sec, long usec);

#endif /* !__ARCH_UM_DRIVERS_CLOCK_H__ */
