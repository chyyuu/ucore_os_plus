#ifndef __GLUE_INTR_H__
#define __GLUE_INTR_H__

/* These are offsets of each field in the trapframe, used by vectors.
 * r0 is not saved on the stack as it is always 0.
 * r1 is saved in SP
 */

#define PC        0
#define SR        4
#define SP        8
#define GPR2      12
#define GPR3      16
#define GPR4      20
#define GPR5      24
#define GPR6      28
#define GPR7      32
#define GPR8      36
#define GPR9      40
#define GPR10     44
#define GPR11     48
#define GPR12     52
#define GPR13     56
#define GPR14     60
#define GPR15     64
#define GPR16     68
#define GPR17     72
#define GPR18     76
#define GPR19     80
#define GPR20     84
#define GPR21     88
#define GPR22     92
#define GPR23     96
#define GPR24     100
#define GPR25     104
#define GPR26     108
#define GPR27     112
#define GPR28     116
#define GPR29     120
#define GPR30     124
#define GPR31     128
#define ORIG_GPR3 132
#define RESULT    136
#define SYSCALLNO 140
#define TF_SIZE   148

/* These are offsets which is used to store gprs temporarily in the first 0x100 area.
 */
#define GPR1_TMP  0xA0
#define GPR2_TMP  0xA4
#define GPR3_TMP  0xA8
#define GPR4_TMP  0xAC
#define GPR5_TMP  0xB0
#define GPR6_TMP  0xB4
#define GPR10_TMP 0xB8
#define GPR13_TMP 0xBC
#define GPR31_TMP 0xC0

#ifndef __ASSEMBLER__

#include <types.h>
#include <arch.h>
#include <intr.h>
#include <sync.h>

#define EXCEPTION_BUS         0x200
#define EXCEPTION_DPAGE_FAULT 0x300
#define EXCEPTION_IPAGE_FAULT 0x400
#define EXCEPTION_TIMER       0x500
#define EXCEPTION_EXTERNAL    0x800
#define EXCEPTION_SYSCALL     0xC00

struct pushregs {
	uint32_t gprs[30];
};

struct trapframe {
	uint32_t pc;
	uint32_t sr;
	uint32_t sp;
	struct pushregs regs;
	uint32_t orig_gpr3;
	uint32_t result;
	uint32_t syscallno;
	uint32_t ea;
};

#define local_intr_enable_hw  intr_enable()
#define local_intr_disable_hw intr_disable()

#define local_intr_save_hw(x)      local_intr_save(x)
#define local_intr_restore_hw(x)   local_intr_restore(x)

#endif /* !__ASSEMBLER__ */

#endif /* !__GLUE_INTR_H__ */
