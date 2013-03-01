#ifndef __ARCH_UM_DIRVERS_TIME_H__
#define __ARCH_UM_DIRVERS_TIME_H__

#define	ITIMER_REAL			0
#define	ITIMER_VIRTUAL		1
#define	ITIMER_PROF			2

typedef long __kernel_time_t;
typedef long __kernel_suseconds_t;

struct timeval {
	__kernel_time_t tv_sec;	/* seconds */
	__kernel_suseconds_t tv_usec;	/* microseconds */
};

struct itimerval {
	struct timeval it_interval;	/* timer interval */
	struct timeval it_value;	/* current value */
};

#endif /* !__ARCH_UM_DIRVERS_TIME_H__ */
