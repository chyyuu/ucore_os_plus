#ifndef __ARCH_OR32_INCLUDE_OR32_H__
#define __ARCH_OR32_INCLUDE_OR32_H__

#include <types.h>
#include <or32/spr_defs.h>

#define NR_GPRS    32

#define REG8(add)  *((volatile unsigned char *)  (add))
#define REG16(add) *((volatile unsigned short *) (add))
#define REG32(add) *((volatile unsigned long *)  (add))

/***************************************************
 * Copied from include/asm-generic/div64.h in Linux 2.39.1
 ***************************************************/

extern uint32_t __div64_32(uint64_t * dividend, uint32_t divisor);

/* The unnecessary pointer compare is there
 * to check for type safety (n must be 64bit)
 */
#define do_div(n,base) ({									\
			uint32_t __base = (base);						\
			uint32_t __rem;									\
			(void)(((typeof((n)) *)0) == ((uint64_t *)0));	\
			if ((((n) >> 32) == 0)) {						\
				__rem = (uint32_t)(n) % __base;				\
				(n) = (uint32_t)(n) / __base;				\
			} else											\
				__rem = __div64_32(&(n), __base);			\
			__rem;											\
		})

/**************************************************
 * The copy ends here.
 **************************************************/

static inline uint32_t mfspr(uint16_t addr) __attribute__ ((always_inline));
static inline void mtspr(uint16_t addr, uint32_t data)
    __attribute__ ((always_inline));

static inline uint32_t mfspr(uint16_t addr)
{
	uint32_t ret;
	asm volatile ("l.mfspr %0, %1, 0":"=r" (ret):"r"(addr));
	return ret;
}

static inline void mtspr(uint16_t addr, uint32_t data)
{
	asm volatile ("l.mtspr %0, %1, 0"::"r" (addr), "r"(data));
}

static inline void enable_interrupt(void) __attribute__ ((always_inline));

static inline void enable_interrupt(void)
{
	/* asm ("l.mfspr r3, r0, %0\n" */
	/*       "l.ori r3, r3, %1\n" */
	/*       "l.mtspr r0, r3, %0\n" */
	/*       :: "i" (SPR_SR), "i" (SPR_SR_IEE)); */
	mtspr(SPR_SR, mfspr(SPR_SR) | SPR_SR_IEE);
}

#endif /* __ARCH_OR32_INCLUDE_OR32_H__ */
