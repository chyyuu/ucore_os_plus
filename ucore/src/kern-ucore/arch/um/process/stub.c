#include <stub.h>
#include <types.h>
#include <memlayout.h>
#include <host_signal.h>
#include <arch.h>
#include <stdio.h>
#include <assert.h>

void __attribute__ ((__section__(".__syscall_stub")))
    stub_segv_handler(int sig)
{
	struct sigcontext *sc = (struct sigcontext *)(&sig + 1);
	struct faultinfo *fi = (struct faultinfo *)(STUB_DATA + 0x800);

	fi->error_code = sc->err | 0x4;
	fi->cr2 = sc->cr2;
	fi->trap_no = sc->trapno;

	trap_myself();
}

void stub_push_frame(struct stub_stack *stack)
{
	stack->current_no++;
	stack->current_addr += sizeof(struct stub_frame);
}

void stub_pop_frame(struct stub_stack *stack)
{
	assert(stack->current_no > 0);

	stack->current_no--;
	stack->current_addr -= sizeof(struct stub_frame);
}
