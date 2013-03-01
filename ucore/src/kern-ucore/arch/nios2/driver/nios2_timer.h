#ifndef __NIOS2_KERN_DRIVER_TIMER_H__
#define __NIOS2_KERN_DRIVER_TIMER_H__

void nios2_timer_init(void);

void nios2_timer_usleep(int usec);

struct np_timer {		//not used...
	int np_timerstatus;	// read only, 2 bits (any write to clear TO)
	int np_timercontrol;	// write/readable, 4 bits
	int np_timerperiodl;	// write/readable, 16 bits
	int np_timerperiodh;	// write/readable, 16 bits
	int np_timersnapl;	// read only, 16 bits
	int np_timersnaph;	// read only, 16 bits
};

extern struct np_timer *na_timer;
extern struct np_timer *na_timer1;

#endif /* !__NIOS2_KERN_DRIVER_TIMER_H__ */
