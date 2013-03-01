#ifndef __ARCH_UM_INCLUDE_ARCH_H__
#define __ARCH_UM_INCLUDE_ARCH_H__

#include <linux/syscall.h>
#include <linux/signal.h>
#include <linux/ptrace.h>
#include <linux/wait.h>
#include <linux/fcntl.h>
#include <linux/ioctl.h>
#include <linux/mmap.h>
#include <linux/time.h>
#include <linux/setjmp.h>
#include <linux/process.h>

#ifndef __ASSEMBLER__

/* Copied from the original x86.h */
#define do_div(n, base) ({										\
	unsigned long __upper, __low, __high, __mod, __base;		\
	__base = (base);											\
	asm("" : "=a" (__low), "=d" (__high) : "A" (n));			\
	__upper = __high;											\
	if (__high != 0) {											\
		__upper = __high % __base;								\
		__high = __high / __base;								\
	}															\
	asm("divl %2" : "=a" (__low), "=d" (__mod)					\
	    : "rm" (__base), "0" (__low), "1" (__upper));			\
	asm("" : "=A" (n) : "a" (__low), "d" (__high));				\
	__mod;														\
 })

void host_exit(int sig);

#endif /* !__ASSEMBLER__ */

#endif /* !__ARCH_UM_INCLUDE_ARCH_H__ */
