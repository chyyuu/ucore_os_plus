#ifndef __KERN_TRAP_TRAP_H__
#define __KERN_TRAP_TRAP_H__

#include <types.h>
#include <glue_intr.h>

void print_trapframe(struct trapframe *tf);
void print_regs(struct pushregs *regs);
bool trap_in_kernel(struct trapframe *tf);
int ucore_in_interrupt();

#endif /* !__KERN_TRAP_TRAP_H__ */
