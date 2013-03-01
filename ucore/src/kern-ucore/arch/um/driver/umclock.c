#include <umclock.h>
#include <arch.h>
#include <stdio.h>

volatile size_t ticks;

/**
 * register virtual timer handler and initialize virtual timer with default interval
 * @return 0 on success, -1 on error when set interval, -2 on error when register timer handler
 */
int umclock_init(void)
{
	return umclock_set_interval(DEFAULT_TV_SEC, DEFAULT_TV_USEC);
}

/**
 * set virtual timer interval and restart the timer
 * @param sec interval in seconds
 * @param usec interval in microseconds
 * @return 0 on success
 */
int umclock_set_interval(long sec, long usec)
{
	struct itimerval interval =
	    ((struct itimerval){ {sec, usec}, {sec, usec} });
	return syscall3(__NR_setitimer, ITIMER_VIRTUAL, (long)&interval, 0);
}
