/*
 * =====================================================================================
 *
 *       Filename:  timer_glue.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/26/2012 12:17:30 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <linux/timer.h>

u64 __jiffy_data jiffies_64;
unsigned long volatile __jiffy_data jiffies;

inline unsigned int jiffies_to_msecs(const unsigned long j)
{
#if HZ <= MSEC_PER_SEC && !(MSEC_PER_SEC % HZ)
	return (MSEC_PER_SEC / HZ) * j;
#elif HZ > MSEC_PER_SEC && !(HZ % MSEC_PER_SEC)
	return (j + (HZ / MSEC_PER_SEC) - 1) / (HZ / MSEC_PER_SEC);
#else
#if BITS_PER_LONG == 32
	return (HZ_TO_MSEC_MUL32 * j) >> HZ_TO_MSEC_SHR32;
#else
	return (j * HZ_TO_MSEC_NUM) / HZ_TO_MSEC_DEN;
#endif
#endif
}

/* HZ defined in autoconfig.h */
/* timer */
unsigned long msecs_to_jiffies(const unsigned int m)
{
	/*
	 * Negative value, means infinite timeout:
	 */
	if ((int)m < 0)
		return MAX_JIFFY_OFFSET;

#if HZ <= MSEC_PER_SEC && !(MSEC_PER_SEC % HZ)
	/*
	 * HZ is equal to or smaller than 1000, and 1000 is a nice
	 * round multiple of HZ, divide with the factor between them,
	 * but round upwards:
	 */
	return (m + (MSEC_PER_SEC / HZ) - 1) / (MSEC_PER_SEC / HZ);
#elif HZ > MSEC_PER_SEC && !(HZ % MSEC_PER_SEC)
	/*
	 * HZ is larger than 1000, and HZ is a nice round multiple of
	 * 1000 - simply multiply with the factor between them.
	 *
	 * But first make sure the multiplication result cannot
	 * overflow:
	 */
	if (m > jiffies_to_msecs(MAX_JIFFY_OFFSET))
		return MAX_JIFFY_OFFSET;

	return m * (HZ / MSEC_PER_SEC);
#else
	/*
	 * Generic case - multiply, round and divide. But first
	 * check that if we are doing a net multiplication, that
	 * we wouldn't overflow:
	 */
	if (HZ > MSEC_PER_SEC && m > jiffies_to_msecs(MAX_JIFFY_OFFSET))
		return MAX_JIFFY_OFFSET;

	return (MSEC_TO_HZ_MUL32 * m + MSEC_TO_HZ_ADJ32)
	    >> MSEC_TO_HZ_SHR32;
#endif
}

void add_input_randomness(unsigned int type, unsigned int code,
			  unsigned int value)
{
	//TODO
}

int mod_timer(struct timer_list *timer, unsigned long expires)
{
	//TODO
	return -1;
}

void init_timer(struct timer_list *timer)
{
}

bool printk_timed_ratelimit(unsigned long *caller_jiffies,
			    unsigned int interval_msecs)
{
	if (*caller_jiffies == 0
	    || !time_in_range(jiffies, *caller_jiffies,
			      *caller_jiffies
			      + msecs_to_jiffies(interval_msecs))) {
		*caller_jiffies = jiffies;
		return true;
	}
	return false;
}

void ktime_get_ts(struct timespec *ts)
{
	int msecs = jiffies_to_msecs(jiffies);
	ts->tv_sec = msecs / MSEC_PER_SEC;
	ts->tv_nsec = msecs * NSEC_PER_MSEC;
}

extern void __ucore_add_timer(void *linux_timer, int expires,
			      unsigned long data,
			      void (*function) (unsigned long));
void linux_add_timer(struct timer_list *timer)
{
	__ucore_add_timer(timer, timer->expires - jiffies, timer->data,
			  timer->function);
}
