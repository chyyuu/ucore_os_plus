#ifndef __ARCH_OR32_INCLUDE_OR32_OR32_FUNC_H__
#define __ARCH_OR32_INCLUDE_OR32_OR32_FUNC_H__

#include <or32/spr_defs.h>
#include <memlayout.h>

#ifdef __ASSEMBLER__

.macro load32i reg const
 l.movhi \ reg, hi(\const) l.ori \ reg, \reg, lo(\const)
.endm
#define SR_ENABLE_BITS(mask, t1, t2)	\
	    l.mfspr t2, r0, SPR_SR	;\
		load32i	t1, (mask)      ;\
	    l.or    t2, t2, t1		;\
	    l.mtspr r0, t2, SPR_SR
#define SR_DISABLE_BITS(mask, t1, t2)	\
	    l.mfspr t2, r0, SPR_SR	;\
		load32i	t1, (~mask)     ;\
	    l.and   t2, t2, t1		;\
	    l.mtspr r0, t2, SPR_SR
#define ENABLE_INTERRUPTS(t1, t2) \
		SR_ENABLE_BITS((SPR_SR_IEE | SPR_SR_TEE), t1, t2)
#define DISABLE_INTERRUPTS(t1, t2)	\
		SR_DISABLE_BITS((SPR_SR_IEE | SPR_SR_TEE), t1, t2)
#define ENABLE_MMU(t1, t2) \
		SR_ENABLE_BITS((SPR_SR_IME | SPR_SR_DME), t1, t2)
/*
 * Transfer virtual address @t2 to physical address stored in @t1
 */
#define tophys(t1, t2)    \
		load32i	t1, -KERNBASE  	;\
		l.add	t1, t1, t2
/*
 * Transfer physical address @t2 to virtual address stored in @t1
 */
#define tovirt(t1, t2)    \
		load32i	t1, KERNBASE	;\
		l.add	t1, t1, t2
#endif				/* !__ASSEMBLER__ */
#endif				/* !__ARCH_OR32_INCLUDE_OR32_OR32_FUNC_H__ */
