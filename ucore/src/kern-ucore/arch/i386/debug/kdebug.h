#ifndef __KERN_DEBUG_KDEBUG_H__
#define __KERN_DEBUG_KDEBUG_H__

#include <types.h>
#include <trap.h>

void print_kerninfo(void);
void print_stackframe(void);
void print_debuginfo(uintptr_t eip);

// number of debug registers
#define MAX_DR_NUM              4

// index of DR6-Debug status and DR7-Debug control
#define DR_STATUS               6
#define DR_CONTROL              7

/* DR6 Register */
#define DR6_B0_BIT              0x00000001
#define DR6_B1_BIT              0x00000002
#define DR6_B2_BIT              0x00000004
#define DR6_B3_BIT              0x00000008

/* DR7 Register */
#define DR7_L0_BIT              0x00000001
#define DR7_G0_BIT              0x00000002
#define DR7_L1_BIT              0x00000004
#define DR7_G1_BIT              0x00000008
#define DR7_L2_BIT              0x00000010
#define DR7_G2_BIT              0x00000020
#define DR7_L3_BIT              0x00000040
#define DR7_G3_BIT              0x00000080
#define DR7_LEXACT              0x00000100
#define DR7_GEXACT              0x00000200
#define DR7_GDETECT             0x00002000
#define DR7_MASK                0xFFFF0000

void debug_init(void);
void debug_monitor(struct trapframe *tf);
void debug_list_dr(void);
int debug_enable_dr(unsigned regnum, uintptr_t addr, unsigned type,
		    unsigned len);
int debug_disable_dr(unsigned regnum);

#endif /* !__KERN_DEBUG_KDEBUG_H__ */
