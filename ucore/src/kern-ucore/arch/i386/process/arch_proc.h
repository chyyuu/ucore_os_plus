#ifndef __ARCH_X86_INCLUDE_ARCH_PROC_H__
#define __ARCH_X86_INCLUDE_ARCH_PROC_H__

#include <types.h>

// Saved registers for kernel context switches.
// Don't need to save all the %fs etc. segment registers,
// because they are constant across kernel contexts.
// Save all the regular registers so we don't need to care
// which are caller save, but not the return register %eax.
// (Not saving %eax just simplifies the switching code.)
// The layout of context must match code in switch.S.
struct context {
	uint32_t eip;
	uint32_t esp;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t esi;
	uint32_t edi;
	uint32_t ebp;
};

struct arch_proc_struct {
};

#endif /* !__ARCH_X86_INCLUDE_ARCH_PROC_H__ */
