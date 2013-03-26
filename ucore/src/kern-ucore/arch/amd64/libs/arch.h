#ifndef __LIBS_X86_H__
#define __LIBS_X86_H__

#include <types.h>
#include "msrbits.h"

#define CACHELINE 64

#define barrier() __asm__ __volatile__ ("" ::: "memory")
#define __noret__   __attribute__((noreturn))

/* maxinum cpu number */
#define MAX_IOAPIC     8

static inline
void hlt(void)
{
  __asm volatile("hlt");
}

static inline
void nop_pause(void)
{
  __asm volatile("pause" : :);
}



static inline uint8_t inb(uint16_t port) __attribute__ ((always_inline));
static inline void insl(uint32_t port, void *addr, int cnt)
    __attribute__ ((always_inline));
static inline void outb(uint16_t port, uint8_t data)
    __attribute__ ((always_inline));
static inline void outw(uint16_t port, uint16_t data)
    __attribute__ ((always_inline));
static inline void outsl(uint32_t port, const void *addr, int cnt)
    __attribute__ ((always_inline));

/* Pseudo-descriptors used for LGDT, LLDT(not used) and LIDT instructions. */
struct pseudodesc {
	uint16_t pd_lim;	// Limit
	uintptr_t pd_base;	// Base address
} __attribute__ ((packed));

static inline void lidt(struct pseudodesc *pd) __attribute__ ((always_inline));
static inline void sti(void) __attribute__ ((always_inline));
static inline void cli(void) __attribute__ ((always_inline));
static inline void ltr(uint16_t sel) __attribute__ ((always_inline));
static inline void lcr0(uintptr_t cr0) __attribute__ ((always_inline));
static inline void lcr3(uintptr_t cr3) __attribute__ ((always_inline));
static inline uintptr_t rcr0(void) __attribute__ ((always_inline));
static inline uintptr_t rcr1(void) __attribute__ ((always_inline));
static inline uintptr_t rcr2(void) __attribute__ ((always_inline));
static inline uintptr_t rcr3(void) __attribute__ ((always_inline));
static inline void invlpg(void *addr) __attribute__ ((always_inline));

static inline uint8_t inb(uint16_t port)
{
	uint8_t data;
	asm volatile ("inb %1, %0":"=a" (data):"d"(port):"memory");
	return data;
}

static inline uint16_t
inw(uint16_t port)
{
  uint16_t data = 0;

  __asm volatile("inw %1,%0" : "=a" (data) : "d" (port));
  return data;
}

static inline uint32_t
inl(uint16_t port)
{
  uint32_t data = 0;

  __asm volatile("inl %w1,%0" : "=a" (data) : "d" (port));
  return data;
}



static inline void insl(uint32_t port, void *addr, int cnt)
{
	asm volatile ("cld;" "repne; insl;":"=D" (addr), "=c"(cnt)
		      :"d"(port), "0"(addr), "1"(cnt)
		      :"memory", "cc");
}

static inline void outb(uint16_t port, uint8_t data)
{
	asm volatile ("outb %0, %1"::"a" (data), "d"(port):"memory");
}

static inline void outw(uint16_t port, uint16_t data)
{
	asm volatile ("outw %0, %1"::"a" (data), "d"(port):"memory");
}

static inline void
outl(uint16_t port, uint32_t data)
{
  __asm volatile("outl %0,%w1" : : "a" (data), "d" (port));
}



static inline void outsl(uint32_t port, const void *addr, int cnt)
{
	asm volatile ("cld;" "repne; outsl;":"=S" (addr), "=c"(cnt)
		      :"d"(port), "0"(addr), "1"(cnt)
		      :"memory", "cc");
}

static inline void lidt(struct pseudodesc *pd)
{
	asm volatile ("lidt (%0)"::"r" (pd):"memory");
}

static inline void sti(void)
{
	asm volatile ("sti");
}

static inline void cli(void)
{
	asm volatile ("cli":::"memory");
}

static inline void ltr(uint16_t sel)
{
	asm volatile ("ltr %0"::"r" (sel):"memory");
}

static inline void lcr0(uintptr_t cr0)
{
	asm volatile ("mov %0, %%cr0"::"r" (cr0):"memory");
}

static inline void lcr3(uintptr_t cr3)
{
	asm volatile ("mov %0, %%cr3"::"r" (cr3):"memory");
}

static inline uintptr_t rcr0(void)
{
	uintptr_t cr0;
	asm volatile ("mov %%cr0, %0":"=r" (cr0)::"memory");
	return cr0;
}

static inline uintptr_t rcr1(void)
{
	uintptr_t cr1;
	asm volatile ("mov %%cr1, %0":"=r" (cr1)::"memory");
	return cr1;
}

static inline uintptr_t rcr2(void)
{
	uintptr_t cr2;
	asm volatile ("mov %%cr2, %0":"=r" (cr2)::"memory");
	return cr2;
}

static inline uintptr_t rcr3(void)
{
	uintptr_t cr3;
	asm volatile ("mov %%cr3, %0":"=r" (cr3)::"memory");
	return cr3;
}

static inline void invlpg(void *addr)
{
	asm volatile ("invlpg (%0)"::"r" (addr):"memory");
}

#ifdef __UCORE_64__

#define do_div(n, base) ({                                          \
            uint64_t __mod, __base = (uint64_t)base;                \
            __mod = ((uint64_t)(n)) % __base;                       \
            (n) = ((uint64_t)(n)) / __base;                         \
            __mod;                                                  \
        })

static inline uint64_t read_rbp(void) __attribute__ ((always_inline));
static inline uint64_t read_rflags(void) __attribute__ ((always_inline));
static inline void write_rflags(uint64_t rflags)
    __attribute__ ((always_inline));

static inline uint64_t read_rbp(void)
{
	uint64_t rbp;
	asm volatile ("movq %%rbp, %0":"=r" (rbp));
	return rbp;
}

static inline uint64_t read_rflags(void)
{
	uint64_t rflags;
	asm volatile ("pushfq; popq %0":"=r" (rflags));
	return rflags;
}

static inline void write_rflags(uint64_t rflags)
{
	asm volatile ("pushq %0; popfq"::"r" (rflags));
}

static inline void
writefs(uint16_t v)
{
  __asm volatile("movw %0, %%fs" : : "r" (v));
}

static inline uint16_t
readfs(void)
{
  uint16_t v;
  __asm volatile("movw %%fs, %0" : "=r" (v));
  return v;
}

static inline void
writegs(uint16_t v)
{
  __asm volatile("movw %0, %%gs" : : "r" (v));
}

static inline uint16_t
readgs(void)
{
  uint16_t v;
  __asm volatile("movw %%gs, %0" : "=r" (v));
  return v;
}

static inline uint64_t
readmsr(uint32_t msr)
{
  uint32_t hi, lo;
  __asm volatile("rdmsr" : "=d" (hi), "=a" (lo) : "c" (msr));
  return ((uint64_t) lo) | (((uint64_t) hi) << 32);
}

static inline void
writemsr(uint64_t msr, uint64_t val)
{
  uint32_t lo = val & 0xffffffff;
  uint32_t hi = val >> 32;
  __asm volatile("wrmsr" : : "c" (msr), "a" (lo), "d" (hi) : "memory");
}

static inline uint64_t
rdtsc(void)
{
  uint32_t hi, lo;
  __asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)lo)|(((uint64_t)hi)<<32);
}

static inline uint64_t
rdpmc(uint32_t ecx)
{
  uint32_t hi, lo;
  __asm volatile("rdpmc" : "=a" (lo), "=d" (hi) : "c" (ecx));
  return ((uint64_t) lo) | (((uint64_t) hi) << 32);
}

static inline uint64_t
rrsp(void)
{
  uint64_t val;
  __asm volatile("movq %%rsp,%0" : "=r" (val));
  return val;
}

static inline uint64_t
rrbp(void)
{
  uint64_t val;
  __asm volatile("movq %%rbp,%0" : "=r" (val));
  return val;
}



#else /* not __UCORE_64__ (only used for 32-bit libs) */

#define do_div(n, base) ({                                          \
            unsigned long __upper, __low, __high, __mod, __base;    \
            __base = (base);                                        \
            asm ("" : "=a" (__low), "=d" (__high) : "A" (n));       \
            __upper = __high;                                       \
            if (__high != 0) {                                      \
                __upper = __high % __base;                          \
                __high = __high / __base;                           \
            }                                                       \
            asm ("divl %2" : "=a" (__low), "=d" (__mod)             \
                : "rm" (__base), "0" (__low), "1" (__upper));       \
            asm ("" : "=A" (n) : "a" (__low), "d" (__high));        \
            __mod;                                                  \
        })

static inline uint32_t read_ebp(void) __attribute__ ((always_inline));
static inline uint32_t read_eflags(void) __attribute__ ((always_inline));
static inline void write_eflags(uint32_t eflags)
    __attribute__ ((always_inline));

static inline uint32_t read_ebp(void)
{
	uint32_t ebp;
	asm volatile ("movl %%ebp, %0":"=r" (ebp));
	return ebp;
}

static inline uint32_t read_eflags(void)
{
	uint32_t eflags;
	asm volatile ("pushfl; popl %0":"=r" (eflags));
	return eflags;
}

static inline void write_eflags(uint32_t eflags)
{
	asm volatile ("pushl %0; popfl"::"r" (eflags));
}


#endif /* !__UCORE_64__ */

static inline int __strcmp(const char *s1, const char *s2)
    __attribute__ ((always_inline));
static inline char *__strcpy(char *dst, const char *src)
    __attribute__ ((always_inline));
static inline void *__memset(void *s, char c, size_t n)
    __attribute__ ((always_inline));
static inline void *__memmove(void *dst, const void *src, size_t n)
    __attribute__ ((always_inline));
static inline void *__memcpy(void *dst, const void *src, size_t n)
    __attribute__ ((always_inline));

#ifndef __HAVE_ARCH_STRCMP
#define __HAVE_ARCH_STRCMP
static inline int __strcmp(const char *s1, const char *s2)
{
	int d0, d1, ret;
	asm volatile ("1: lodsb;"
		      "scasb;"
		      "jne 2f;"
		      "testb %%al, %%al;"
		      "jne 1b;"
		      "xorl %%eax, %%eax;"
		      "jmp 3f;"
		      "2: sbbl %%eax, %%eax;"
		      "orb $1, %%al;" "3:":"=a" (ret), "=&S"(d0), "=&D"(d1)
		      :"1"(s1), "2"(s2)
		      :"memory");
	return ret;
}

#endif /* __HAVE_ARCH_STRCMP */

#ifndef __HAVE_ARCH_STRCPY
#define __HAVE_ARCH_STRCPY
static inline char *__strcpy(char *dst, const char *src)
{
	int d0, d1, d2;
	asm volatile ("1: lodsb;"
		      "stosb;"
		      "testb %%al, %%al;"
		      "jne 1b;":"=&S" (d0), "=&D"(d1), "=&a"(d2)
		      :"0"(src), "1"(dst):"memory");
	return dst;
}
#endif /* __HAVE_ARCH_STRCPY */

#ifndef __HAVE_ARCH_MEMSET
#define __HAVE_ARCH_MEMSET
static inline void *__memset(void *s, char c, size_t n)
{
	int d0, d1;
	asm volatile ("rep; stosb;":"=&c" (d0), "=&D"(d1)
		      :"0"(n), "a"(c), "1"(s)
		      :"memory");
	return s;
}
#endif /* __HAVE_ARCH_MEMSET */

#ifndef __HAVE_ARCH_MEMMOVE
#define __HAVE_ARCH_MEMMOVE
static inline void *__memmove(void *dst, const void *src, size_t n)
{
	if (dst < src) {
		return __memcpy(dst, src, n);
	}
	int d0, d1, d2;
	asm volatile ("std;"
		      "rep; movsb;" "cld;":"=&c" (d0), "=&S"(d1), "=&D"(d2)
		      :"0"(n), "1"(n - 1 + src), "2"(n - 1 + dst)
		      :"memory");
	return dst;
}
#endif /* __HAVE_ARCH_MEMMOVE */

#ifndef __HAVE_ARCH_MEMCPY
#define __HAVE_ARCH_MEMCPY
static inline void *__memcpy(void *dst, const void *src, size_t n)
{
	int d0, d1, d2;
	asm volatile ("rep; movsl;"
		      "movl %4, %%ecx;"
		      "andl $3, %%ecx;"
		      "jz 1f;"
		      "rep; movsb;" "1:":"=&c" (d0), "=&D"(d1), "=&S"(d2)
		      :"0"(n / 4), "m"(n), "1"(dst), "2"(src)
		      :"memory");
	return dst;
}
#endif /* __HAVE_ARCH_MEMCPY */

#endif /* !__LIBS_X86_H__ */
