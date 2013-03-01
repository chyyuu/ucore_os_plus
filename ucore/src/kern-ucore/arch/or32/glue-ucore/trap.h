#ifndef __GLUE_UCORE_TRAP_H__
#define __GLUE_UCORE_TRAP_H__

#include <glue_intr.h>

#ifndef __ASSEMBLER__

#include <types.h>

void print_trapframe(struct trapframe *tf);
void print_regs(struct pushregs *regs);
bool trap_in_kernel(struct trapframe *tf);

#endif /* !__ASSEMBLER__ */

#endif
