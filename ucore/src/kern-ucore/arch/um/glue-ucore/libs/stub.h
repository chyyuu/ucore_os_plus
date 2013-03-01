#ifndef __ARCH_UM_INCLUDE_STUB_H__
#define __ARCH_UM_INCLUDE_STUB_H__

#include <types.h>

/*
 * Some actions must be carried out by the container process like map/unmap a range of space.
 * The 'stub code' is used to deal with these requirements, and 'stub stack' stores information needed.
 * The info is organized on a stack because another stub call may be needed when another is being handled.
 *     (eg. To touch a page may need loading it first.)
 * Here's what the stack looks like:
 *
 *    +------------------------------+ <---------- STUB_DATA
 *	  |   address of current frame   |
 *	  +------------------------------+
 *	  | serial No. of current frame  |
 *	  +------------------------------+
 *	  |                              |
 *	  |        Stub Frame #0         |
 *	  |                              |
 *	  +------------------------------+
 *	  |                              |
 *	  |        Stub Frame #1         |
 *	  |                              |
 *	  +------------------------------+
 *	  |                              |
 *	  :                              :
 *	  |                              |
 *	  +------------------------------+ <---------- STUB_END
 *
 * Address is used by the stub interpreting code (see stub_exec_syscall.S) which determines what to do
 *     by eax of the current frame and executes the syscall/read/write.
 * Serial No. is used by the kernel to keep track of how many frames there are in the stack.
 * Note that the page STUB_DATA is writable by the user (to fill eax with the return value)
 *     and may be visited illegally.
 *
 */

/**
 * A stub frame.
 *   The registers are filled into the container process before the stub call.
 *     For we have a copy in the PCB, we can always restore the right values to the container.
 *   The field 'data' is used when there're more than five arguments for the syscall. (eg. mmap)
 *     Under that circumstance, ebx store the address of the field 'data'.
 */
struct stub_frame {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
	uint32_t esi;
	uint32_t edi;
	uint32_t data[10];
};

struct stub_stack {
	uintptr_t current_addr;
	uint32_t current_no;
	void *frames;
};

#define current_stub_frame(stack)						\
	((struct stub_frame *)&(stack->frames) + stack->current_no)

extern void *__syscall_stub_start;
extern void *__syscall_stub_end;

void stub_entry(void);
void stub_start(void);
void stub_segv_handler(int sig);
void stub_exec_syscall();

void stub_push_frame(struct stub_stack *stack);
void stub_pop_frame(struct stub_stack *stack);

#endif // __ARCH_UM_INCLUDE_STUB_H__
