#ifndef __GLUE_UCORE_TRAP_H__
#define __GLUE_UCORE_TRAP_H__

#include <types.h>
#include <glue_intr.h>

void trap_init(void);
void trap_init_ap(void);
void print_trapframe(struct trapframe *tf);
void print_regs(struct pushregs *regs);
bool trap_in_kernel(struct trapframe *tf);

#endif
