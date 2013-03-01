#ifndef __ARCH_UM_ARCH_PROC_H__
#define __ARCH_UM_ARCH_PROC_H__

#include <arch.h>
#include <types.h>
#include <stub.h>

struct proc_struct;

#define RUN_AS_USER								\
	(current != NULL && current->arch.regs.is_user == 1)

/* The context needed for switching */
struct context {
	jmp_buf switch_buf;	/* Info needed for thread switching */
};

/* The container process descriptor. */
struct host_container {
	uint32_t host_pid;	/* The pid of the container in the host system */
	uint32_t nr_threads;	/* Number of threads the process contains */
	struct stub_stack *stub_stack;	/* The address of the stub stack of the container in kernel */
};

/* The architecture-dependent part of the PCB */
struct arch_proc_struct {
	int forking;		/* This is of no use now. */
	struct host_container *host;	/* The container process descriptor of the user process */
	struct um_pt_regs regs;	/* The registers of the container when it is stopped last time */
};

int user_segv(int pid, struct um_pt_regs *regs);
int start_userspace(void *stub_stack);
void userspace(struct um_pt_regs *regs);
void kill_ptraced_process(int pid, int wait_done);

#endif /* !__ARCH_UM_ARCH_PROC_H__ */
