#ifndef __ARCH_ARM_INCLUDE_ARCH_PROC_H__
#define __ARCH_ARM_INCLUDE_ARCH_PROC_H__

#include <types.h>

// Saved registers for kernel context switches.
// Don't need to save all the %fs etc. segment registers,
// because they are constant across kernel contexts.
// Save all the regular registers so we don't need to care
// which are caller save, but not the return register %eax.
// (Not saving %eax just simplifies the switching code.)
// The layout of context must match code in switch.S.
// TODO: select relevant registers to keep
struct context {
	uint32_t r4;
	uint32_t r5;
	uint32_t r6;
	uint32_t r7;
	uint32_t r8;
	uint32_t r9;
	uint32_t sl;
	uint32_t fp;
	uint32_t eic;
	uint32_t esp;
	uint32_t epc;		//lr
	uint32_t e_cpsr;
};

struct arch_proc_struct {
};

#endif /* !__KERN_PROCESS_PROC_H__ */
