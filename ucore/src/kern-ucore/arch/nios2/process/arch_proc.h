#ifndef __ARCH_NIOS2_INCLUDE_ARCH_PROC_H__
#define __ARCH_NIOS2_INCLUDE_ARCH_PROC_H__

#include <types.h>
#include <mmu.h>

struct context {
	uint32_t sp;
	uint32_t r1;
	uint32_t r2;
	uint32_t r3;
//    uint32_t r4;  r4 & r5 are arguments in switch_to(), do not need to save them.
//    uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t r10;
	uint32_t r11;
	uint32_t r12;
	uint32_t r13;
	uint32_t r14;
	uint32_t r15;
	uint32_t r16;
	uint32_t r17;
	uint32_t r18;
	uint32_t r19;
	uint32_t r20;
	uint32_t r21;
	uint32_t r22;
	uint32_t r23;
	uint32_t fp;
	uint32_t ea;
	uint32_t ra;
	uint32_t gp;
	uint32_t status;
	uint32_t r30;
};

struct arch_proc_struct {
};

#endif /* !__ARCH_NIOS2_INCLUDE_ARCH_PROC_H__ */
