#ifndef __ARCH_UM_INCLUDE_HOST_SIGNAL_H__
#define __ARCH_UM_INCLUDE_HOST_SIGNAL_H__

#include <arch.h>
#include <types.h>

/**************************************************
 * signal context related structures
 *     copied from asm/sigcontext.h
 **************************************************/
struct _fpx_sw_bytes {
	uint32_t magic1;	/* FP_XSTATE_MAGIC1 */
	uint32_t extended_size;	/* total size of the layout referred by
				 * fpstate pointer in the sigcontext.
				 */
	uint64_t xstate_bv;
	/* feature bit mask (including fp/sse/extended
	 * state) that is present in the memory
	 * layout.
	 */
	uint32_t xstate_size;	/* actual xsave state size, based on the
				 * features saved in the layout.
				 * 'extended_size' will be greater than
				 * 'xstate_size'.
				 */
	uint32_t padding[7];	/*  for future use. */
};

struct _fpreg {
	unsigned short significand[4];
	unsigned short exponent;
};

struct _fpxreg {
	unsigned short significand[4];
	unsigned short exponent;
	unsigned short padding[3];
};

struct _xmmreg {
	unsigned long element[4];
};

struct _fpstate {
	/* Regular FPU environment */
	unsigned long cw;
	unsigned long sw;
	unsigned long tag;
	unsigned long ipoff;
	unsigned long cssel;
	unsigned long dataoff;
	unsigned long datasel;
	struct _fpreg _st[8];
	unsigned short status;
	unsigned short magic;	/* 0xffff = regular FPU data only */

	/* FXSR FPU environment */
	unsigned long _fxsr_env[6];	/* FXSR FPU env is ignored */
	unsigned long mxcsr;
	unsigned long reserved;
	struct _fpxreg _fxsr_st[8];	/* FXSR FPU reg data is ignored */
	struct _xmmreg _xmm[8];
	unsigned long padding1[44];

	union {
		unsigned long padding2[12];
		struct _fpx_sw_bytes sw_reserved;	/* represents the extended state info */
	};
};

struct sigcontext {
	unsigned short gs, __gsh;
	unsigned short fs, __fsh;
	unsigned short es, __esh;
	unsigned short ds, __dsh;
	unsigned long edi;
	unsigned long esi;
	unsigned long ebp;
	unsigned long esp;
	unsigned long ebx;
	unsigned long edx;
	unsigned long ecx;
	unsigned long eax;
	unsigned long trapno;
	unsigned long err;
	unsigned long eip;
	unsigned short cs, __csh;
	unsigned long eflags;
	unsigned long esp_at_signal;
	unsigned short ss, __ssh;
	struct _fpstate *fpstate;
	unsigned long oldmask;
	unsigned long cr2;
};

/**************************************************
 * signal context related structures end
 **************************************************/

int segv_handler(int sig, struct um_pt_regs *regs);
int io_handler(int sig, struct um_pt_regs *regs);
int vtimer_handler(int sig, struct um_pt_regs *regs);

void host_signal_init();

#endif /* !__ARCH_UM_INCLUDE_HOST_SIGNAL_H__ */
