#ifndef __BITSEARCH_H__
#define __BITSEARCH_H__

#include <libs/types.h>

static inline int bsf(uint64_t n) __attribute__ ((always_inline));
static inline int bsr(uint64_t n) __attribute__ ((always_inline));

static inline int bsf(uint64_t n)
{
	if (n == 0)
		return -1;
	uint64_t result;
	__asm __volatile("bsfq %1, %0":"=r"(result)
			 :"r"(n));
	return result;
}

static inline int bsr(uint64_t n)
{
	if (n == 0)
		return -1;
	uint64_t result;
	__asm __volatile("bsrq %1, %0":"=r"(result)
			 :"r"(n));
	return result;
}

#endif
