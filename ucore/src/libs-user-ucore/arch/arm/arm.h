#ifndef __LIBS_ARM_H__
#define __LIBS_ARM_H__

#include <types.h>
#include <div64.h>
#include <string.h>

//#define BYPASS_CHECK
#define BYPASS_CHECK_SLAB

static inline uint8_t inb(uint32_t port) __attribute__ ((always_inline));
static inline void outb(uint32_t port, uint8_t data)
    __attribute__ ((always_inline));
static inline uint32_t inw(uint32_t port) __attribute__ ((always_inline));
static inline void outw(uint32_t port, uint32_t data)
    __attribute__ ((always_inline));
static inline void irq_flag_enable(void) __attribute__ ((always_inline));
static inline void irq_flag_disable(void) __attribute__ ((always_inline));

static inline uint32_t read_psrflags(void) __attribute__ ((always_inline));
static inline void write_psrflags(uint32_t psrflags)
    __attribute__ ((always_inline));
static inline void invalidate_itlb_line(void *addr)
    __attribute__ ((always_inline));
static inline void invalidate_dtlb_line(void *addr)
    __attribute__ ((always_inline));
static inline uint32_t far(void) __attribute__ ((always_inline));
static inline uint32_t fsr(void) __attribute__ ((always_inline));

static inline uint8_t inb(uint32_t port)
{
	uint8_t data = *((volatile uintptr_t *)port);
	return data;
}

static inline void outb(uint32_t port, uint8_t data)
{
	*((volatile uintptr_t *)port) = data;
}

static inline uint32_t inw(uint32_t port)
{
	uint32_t data = *((volatile uintptr_t *)port);
	return data;
}

static inline void outw(uint32_t port, uint32_t data)
{
	*((volatile uintptr_t *)port) = data;
}

/* ebp is the frame pointer on x86. Its equivalent is fp on ARM */
static inline uint32_t read_fp(void)
{
	uint32_t fp;
	asm volatile ("mov %0, fp":"=r" (fp));
	return fp;
}

/* equivalent of sti assembly instruction */
static inline void irq_flag_enable(void)
{
	asm volatile ("mrs r0, cpsr;"
		      "bic r0, r0, #0x80;"
		      "msr cpsr, r0;":::"r0", "memory", "cc");
}

/* equivalent of cti assembly instruction */
static inline void irq_flag_disable(void)
{
	asm volatile ("mrs r0, cpsr;"
		      "orr r0, r0, #0x80;"
		      "msr cpsr, r0;":::"r0", "memory", "cc");
}

static inline uint32_t read_psrflags(void)
{
	uint32_t psrflags;
	asm volatile ("mrs %0, cpsr":"=r" (psrflags));
	return psrflags;
}

static inline void write_psrflags(uint32_t psrflags)
{
	asm volatile ("msr cpsr, %0"::"r" (psrflags));
}

/* eauivalent of invlpg, hozever there is D TLB and I TLB */
static inline void invalidate_itlb_line(void *addr)
{
	uint32_t c8format = (uint32_t) addr;	/* set bits that change */
	asm volatile ("MCR p15, 0, %0, c8, c5, 1"	/* write tlb flush command */
		      ::"r" (c8format)
	    );
}

static inline void invalidate_dtlb_line(void *addr)
{
	uint32_t c8format = (uint32_t) addr;	/* set bits that change */
	asm volatile ("MCR p15, 0, %0, c8, c6, 1"	/* write tlb flush command */
		      ::"r" (c8format)
	    );
}

static inline uint32_t far(void)
{
	uint32_t c6format;
	asm volatile ("MRC p15, 0, %0, c6, c0, 0;"	//read data in fault address register
		      :"=r" (c6format));
	return c6format;
}

static inline uint32_t fsr(void)
{
	uint32_t c5format;
	asm volatile ("MRC p15, 0, %0, c5, c0, 0;"	//read data in fault status register
		      :"=r" (c5format));
	return c5format;
}

/* ttbSet
 * sets the TTB of the master L1 page table. equivalent of lcr3 */
inline static void ttbSet(uint32_t ttb)
{
	ttb &= 0xffffc000;
	asm volatile ("MCR p15, 0, %0, c2, c0, 0"	/* set translation table base */
		      ::"r" (ttb)
	    );
}

// TODO BELOW
// The implementation below are not inline and should not be used
static inline void *__memset(void *s, char c, size_t n)
    __attribute__ ((always_inline));
static inline void *__memmove(void *dst, const void *src, size_t n)
    __attribute__ ((always_inline));
static inline void *__memcpy(void *dst, const void *src, size_t n)
    __attribute__ ((always_inline));

#ifndef __HAVE_ARCH_MEMSET_TODELETE
#define __HAVE_ARCH_MEMSET_TODELETE
static inline void *__memset(void *s, char c, size_t n)
{
	char *p = s;
	while (n-- > 0) {
		*p++ = c;
	}
	return s;
}
#endif /* __HAVE_ARCH_MEMSET */

#ifndef __HAVE_ARCH_MEMMOVE_TODELETE
#define __HAVE_ARCH_MEMMOVE_TODELETE
static inline void *__memmove(void *dst, const void *src, size_t n)
{
	if (dst < src) {
		return __memcpy(dst, src, n);
	}
	const char *s = src;
	char *d = dst;
	if (s < d && s + n > d) {
		s += n, d += n;
		while (n-- > 0) {
			*--d = *--s;
		}
	} else {
		while (n-- > 0) {
			*d++ = *s++;
		}
	}
	return dst;
}
#endif /* __HAVE_ARCH_MEMMOVE */

#ifndef __HAVE_ARCH_MEMCPY_TODELETE
#define __HAVE_ARCH_MEMCPY_TODELETE
static inline void *__memcpy(void *dst, const void *src, size_t n)
{
	const char *s = src;
	char *d = dst;
	while (n-- > 0) {
		*d++ = *s++;
	}
	return dst;
}
#endif /* __HAVE_ARCH_MEMCPY */

#endif /* !__LIBS_ARM_H__ */
