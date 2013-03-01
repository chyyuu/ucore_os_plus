#include <proc.h>
#include <slab.h>
#include <string.h>
#include <sync.h>
#include <pmm.h>
#include <error.h>
#include <sched.h>
#include <elf.h>
#include <vmm.h>
#include <trap.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <arch.h>
#include <unistd.h>
#include <nios2.h>

void forkrets(struct trapframe *tf);

// alloc_proc - create a proc struct and init fields
struct proc_struct *alloc_proc(void)
{
	struct proc_struct *proc = kmalloc(sizeof(struct proc_struct));
	if (proc != NULL) {
		proc->state = PROC_UNINIT;
		proc->pid = -1;
		proc->runs = 0;
		proc->kstack = 0;
		proc->need_resched = 0;
		proc->parent = NULL;
		proc->mm = NULL;
		memset(&(proc->context), 0, sizeof(struct context));
		proc->tf = NULL;
		proc->cr3 = boot_cr3;
		proc->flags = 0;
		memset(proc->name, 0, PROC_NAME_LEN);
		proc->wait_state = 0;
		proc->cptr = proc->optr = proc->yptr = NULL;
		list_init(&(proc->thread_group));
		proc->rq = NULL;
		list_init(&(proc->run_link));
		proc->time_slice = 0;
		proc->sem_queue = NULL;
		event_box_init(&(proc->event_box));
		proc->fs_struct = NULL;
	}
	return proc;
}

// forkret -- the first kernel entry point of a new thread/process
// NOTE: the addr of forkret is setted in copy_thread function
//       after switch_to, the current proc will execute here.
static void forkret(void)
{
	forkrets(pls_read(current)->tf);
}

// kernel_thread - create a kernel thread using "fn" function
// NOTE: the contents of temp trapframe tf will be copied to 
//       proc->tf in do_fork-->copy_thread function
int kernel_thread(int (*fn) (void *), void *arg, uint32_t clone_flags)
{
	struct trapframe tf;
	memset(&tf, 0, sizeof(struct trapframe));
	tf.tf_regs.r3 = (uint32_t) fn;
	tf.tf_regs.r4 = (uint32_t) arg;
	tf.ea = (uint32_t) kernel_thread_entry;

	return do_fork(clone_flags | CLONE_VM, 0, &tf);
}

int ucore_kernel_thread(int (*fn) (void *), void *arg, uint32_t clone_flags)
{
	kernel_thread(fn, arg, clone_flags);
}

void de_thread_arch_hook(struct proc_struct *proc)
{
}

// copy_thread - setup the trapframe on the  process's kernel stack top and
//             - setup the kernel entry point and stack of process
int
copy_thread(uint32_t clone_flags, struct proc_struct *proc,
	    uintptr_t user_stack, struct trapframe *tf)
{
	proc->tf = (struct trapframe *)(proc->kstack + KSTACKSIZE) - 1;
	*(proc->tf) = *tf;
	proc->tf->tf_regs.r2 = 0;
	proc->tf->sp = tf->sp;
	proc->tf->estatus |= NIOS2_STATUS_PIE_MSK;

	proc->context.ra = (uintptr_t) forkret;
	proc->context.sp = (uintptr_t) (proc->tf);
	return 0;
}

int do_execve_arch_hook(int argc, char **kargv)
{
	return 0;
}

int
init_new_context(struct proc_struct *proc, struct elfhdr *elf,
		 int argc, char **kargv, int envc, char **kenvp)
{
	uintptr_t stacktop = USTACKTOP - argc * PGSIZE;
	char **uargv = (char **)(stacktop - argc * sizeof(char *));
	int i;
	for (i = 0; i < argc; i++) {
		uargv[i] = strcpy((char *)(stacktop + i * PGSIZE), kargv[i]);
	}
	stacktop = (uintptr_t) uargv - sizeof(int);
	*(int *)stacktop = argc;

	struct trapframe *tf = proc->tf;
	memset(tf, 0, sizeof(struct trapframe));

	tf->sp = stacktop;
	tf->ea = elf->e_entry;
	tf->estatus = NIOS2_STATUS_U_MSK | NIOS2_STATUS_PIE_MSK;
	tf->tf_regs.r4 = argc;
	tf->tf_regs.r5 = (uintptr_t) uargv;

	return 0;
}

// kernel_execve - do SYS_exec syscall to exec a user program called by user_main kernel_thread
int kernel_execve(const char *name, const char **argv, const char **kenvp)
{
	int argc = 0;
	while (argv[argc] != NULL) {
		argc++;
	}

	int len = strlen(name);

	register uint32_t __res __asm__("r2");
	register uint32_t __sc __asm__("r4") = (uint32_t) SYS_exec;
	register uint32_t __a __asm__("r5") = (uint32_t) name;
	register uint32_t __d __asm__("r6") = (uint32_t) argv;
	register uint32_t __e __asm__("r7") = (uint32_t) kenvp;

	__asm__ __volatile__("trap":"=r"(__res)
			     :"0"(__sc), "r"(__a), "r"(__d), "r"(__e)
			     :"memory");

	return __res;
}

// cpu_idle - at the end of kern_init, the first kernel thread idleproc will do below works
void cpu_idle(void)
{
	while (1) {
		if (pls_read(current)->need_resched) {
			schedule();
		}
	}
}
