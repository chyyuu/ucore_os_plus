#ifndef __GLUE_UCORE_TRAP_H__
#define __GLUE_UCORE_TRAP_H__

#include <arch.h>
#include <types.h>
#include <glue_intr.h>

int wait_stub_done(int pid);
int get_faultinfo(int pid, struct faultinfo *fi);
int nullify_syscall(int pid, struct um_pt_regs *regs);

void trap_init(void);
void trap_init_ap(void);
void print_trapframe(struct trapframe *tf);
bool trap_in_kernel(struct trapframe *tf);

#endif
