#ifndef __KERN_ARCH_PROCESS_PROC_H__
#define __KERN_ARCH_PROCESS_PROC_H__

#include <defs.h>
#include <list.h>
#include <trap.h>
#include <memlayout.h>

struct context {
	uint32_t sf_s0;
	uint32_t sf_s1;
	uint32_t sf_s2;
	uint32_t sf_s3;
	uint32_t sf_s4;
	uint32_t sf_s5;
	uint32_t sf_s6;
	uint32_t sf_s7;
	uint32_t sf_s8;
	uint32_t sf_gp;
	uint32_t sf_ra;
	uint32_t sf_sp;
};
struct arch_proc_struct {
};

#endif /* !__KERN_ARCH_PROCESS_PROC_H__ */
