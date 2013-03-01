/*
 * =====================================================================================
 *
 *       Filename:  kgdb-stub.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/31/2012 12:42:52 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#ifndef __DEBUG_KGDB_STUB_H
#define __DEBUG_KGDB_STUB_H

/* EABI bp */
#define KGDB_BP_INSTR 0xe7f001f0

#define _GP_REGS		16
#define _FP_REGS		8
#define _EXTRA_REGS		2
#define GDB_MAX_REGS		(_GP_REGS + (_FP_REGS * 3) + _EXTRA_REGS)

#define KGDB_MAX_NO_CPUS	1
#define BUFMAX			400
#define NUMREGBYTES		(GDB_MAX_REGS << 2)
#define NUMCRITREGBYTES		(32 << 2)
#define _CPSR			(GDB_MAX_REGS - 1)

void kgdb_init();
int setup_bp(uint32_t addr);
int remove_bp(uint32_t addr);
int kgdb_trap(struct trapframe *tf);

static inline void kgdb_breakpoint()
{
	__asm__ volatile (".word 0xe7f001f0");
}

#endif
