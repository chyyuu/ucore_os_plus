#ifndef __LIBS_ARM_H__
#define __LIBS_ARM_H__

#include <types.h>
#include <div64.h>
#include <string.h>

//#define BYPASS_CHECK
//#define BYPASS_CHECK_SLAB

#define CACHELINE 64

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

/* flush and clean D/I cache */
inline static void flush_clean_cache(void)
{
	//FIXME
#if 0
	unsigned int c7format = 0;
	asm volatile ("1: mrc p15,0, r15, c7, c14, 3 \n" "bne 1b \n" "mcr p15, 0, %0, c7, c5, 0 \n" "mcr p15, 0, %0, c7, c10, 4 \n"::"r" (c7format):"memory");	/* flush D-cache */
#endif
}

/* ttbSet
 * sets the TTB of the master L1 page table. equivalent of lcr3 */
inline static void ttbSet(uint32_t ttb)
{
	flush_clean_cache();
	ttb &= 0xffffc000;
	asm volatile ("MCR p15, 0, %0, c2, c0, 0"	/* set translation table base */
		      ::"r" (ttb)
	    );
}

#define ARM_SR_MODE_ABT   0x17
#define ARM_SR_MODE_FIQ   0x11
#define ARM_SR_MODE_IRQ   0x12
#define ARM_SR_MODE_SVC   0x13
#define ARM_SR_MODE_SYS   0x1F
#define ARM_SR_MODE_UND   0x1B
#define ARM_SR_MODE_USR   0x10

#define ARM_SR_I          (1<<7)
#define ARM_SR_F          (1<<6)
#define ARM_SR_T          (1<<5)

//in ./libs
void *__memset(void *s, char c, size_t n) __attribute__ ((always_inline));
void *__memmove(void *dst, const void *src, size_t n)
    __attribute__ ((always_inline));
void *__memcpy(void *dst, const void *src, size_t n)
    __attribute__ ((always_inline));

#define __HAVE_ARCH_MEMSET
#define __HAVE_ARCH_MEMMOVE
#define __HAVE_ARCH_MEMCPY

#endif /* !__LIBS_ARM_H__ */
