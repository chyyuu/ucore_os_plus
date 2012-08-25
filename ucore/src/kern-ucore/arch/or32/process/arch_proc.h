#ifndef __ARCH_TEMPLATE_INCLUDE_ARCH_PROC_H__
#define __ARCH_TEMPLATE_INCLUDE_ARCH_PROC_H__

#include <types.h>

/* !TODO: define the context for process switch. */
struct context {
	uint32_t gprs[31];
};

/* !TODO: place any architecture-dependent PCB info here */
struct arch_proc_struct {
};

#endif /* !__ARCH_TEMPLATE_INCLUDE_ARCH_PROC_H__ */
