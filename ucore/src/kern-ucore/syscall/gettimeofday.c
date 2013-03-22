#include "linux_misc_struct.h"
#include <vmm.h>
#include <proc.h>
#include <string.h>

extern int ticks;
int ucore_gettimeofday(struct linux_timeval __user * tv,
		       struct linux_timezone __user * tz)
{
	struct mm_struct *mm = current->mm;
	struct linux_timeval ktv;
	ktv.tv_sec = ticks / 100;
	ktv.tv_usec = (ticks % 100) * 10000;
	lock_mm(mm);
	if (!copy_to_user(mm, tv, &ktv, sizeof(struct linux_timeval))) {
		unlock_mm(mm);
		return -1;
	}
	unlock_mm(mm);
	if (tz) {
		struct linux_timezone ktz;
		memset(&ktz, 0, sizeof(struct linux_timezone));
		lock_mm(mm);
		if (!copy_to_user(mm, tz, &ktz, sizeof(struct linux_timezone))) {
			unlock_mm(mm);
			return -1;
		}
		unlock_mm(mm);
	}
	return 0;
}

int do_clock_gettime(struct linux_timespec __user * time)
{
	struct mm_struct *mm = current->mm;
	struct linux_timespec ktv;
	ktv.tv_sec = ticks / 100;
	ktv.tv_nsec = (ticks % 100) * 10000000;
	lock_mm(mm);
	if (!copy_to_user(mm, time, &ktv, sizeof(struct linux_timespec))) {
		unlock_mm(mm);
		return -1;
	}
	unlock_mm(mm);
	return 0;
}
