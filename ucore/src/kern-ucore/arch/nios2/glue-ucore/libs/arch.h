#ifndef __LIBS_NIOS2_H__
#define __LIBS_NIOS2_H__

//#include <defs.h>
#include <stdio.h>
#include <tlb.h>
#include <pmm.h>
#include <kio.h>

#define do_div(n, base) ({                                          \
            unsigned long __mod;                                    \
            __mod = (n) % (base);                                   \
            (n) /= (base);                                          \
            __mod;                                                  \
        })

uintptr_t nios2_cr3;
#define NIOS2_PGDIR ((pde_t *)nios2_cr3)

static inline void lcr3(uintptr_t cr3) __attribute__ ((always_inline));
static inline uintptr_t rcr3(void) __attribute__ ((always_inline));

static inline void lcr3(uintptr_t cr3)
{
	//kernel?
	tlb_init();
	//kprintf("lcr3: cr3=0x%x KADDR(cr3)=0x%x\n", cr3, KADDR(cr3));
	nios2_cr3 = (uintptr_t) KADDR(cr3);
}

static inline uintptr_t rcr3(void)
{
	return nios2_cr3;
}

#endif /* !__LIBS_NIOS2_H__ */
