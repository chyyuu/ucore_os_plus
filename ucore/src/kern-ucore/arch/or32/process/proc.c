#include <proc.h>
#include <mmu.h>
#include <vmm.h>
#include <slab.h>
#include <trap.h>
#include <arch.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>

#define current (pls_read(current))

void forkrets(struct trapframe *tf);

void forkret(void)
{
	forkrets(current->tf);
}

// alloc_proc - create a proc struct and init fields
struct proc_struct *alloc_proc(void)
{
	struct proc_struct *proc =
	    (struct proc_struct *)kmalloc(sizeof(struct proc_struct));
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
		proc->cr3 = boot_pgdir_pa;
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

// kernel_thread - create a kernel thread using "fn" function
// NOTE: the contents of temp trapframe tf will be copied to 
//       proc->tf in do_fork-->copy_thread function
int kernel_thread(int (*fn) (void *), void *arg, uint32_t clone_flags)
{
	struct trapframe tf;
	memset(&tf, 0, sizeof(struct trapframe));
	tf.regs.gprs[1] = (uint32_t) fn;
	tf.regs.gprs[2] = (uint32_t) arg;
	tf.pc = (uint32_t) kernel_thread_entry;
	tf.sr = SPR_SR_SM;	/* Just for 'copy_thread' to determine whether the new stack should be used. */
	return do_fork(clone_flags | CLONE_VM, 0, &tf);
}

void de_thread_arch_hook(struct proc_struct *proc)
{
}

// copy_thread - setup the trapframe on the  process's kernel stack top and
//             - setup the kernel entry point and stack of process
int
copy_thread(uint32_t clone_flags, struct proc_struct *proc,
	    uintptr_t esp, struct trapframe *tf)
{
	proc->tf = (struct trapframe *)(proc->kstack + KSTACKSIZE) - 1;
	*(proc->tf) = *tf;
	proc->tf->regs.gprs[9] = 0;
	proc->tf->sr = SPR_SR_IME | SPR_SR_DME | SPR_SR_TEE | SPR_SR_IEE;
	if (tf->sr & SPR_SR_SM) {
		proc->tf->sp = (uintptr_t) (proc->tf) + sizeof(proc->tf);
		proc->tf->sr |= SPR_SR_SM;	/* threads forked by kernel should run in supervisor mode */
	} else {
		proc->tf->sp = esp;
	}

	proc->context.gprs[8] = (uintptr_t) forkret;	/* r9 stores the return address */
	proc->context.gprs[0] = (uintptr_t) (proc->tf);	/* r1 stores the stack top pointer */

	return 0;
}

#include <kio.h>

int
init_new_context(struct proc_struct *proc, struct elfhdr *elf, int argc,
		 char **kargv)
{
	uintptr_t stacktop = USTACKTOP - argc * PGSIZE;
	char **uargv = (char **)(stacktop - argc * sizeof(char *));
	int i;
	for (i = 0; i < argc; i++) {
		uargv[i] =
		    (char *)strcpy((char *)(stacktop + i * PGSIZE), kargv[i]);
	}
	stacktop = (uintptr_t) uargv;
	//*(int *)stacktop = argc;

	struct trapframe *tf = proc->tf;
	memset(tf, 0, sizeof(struct trapframe));
	tf->sp = stacktop;
	tf->pc = elf->e_entry;
	tf->sr = SPR_SR_IME | SPR_SR_DME | SPR_SR_TEE | SPR_SR_IEE;
	/* r3 = argc, r4 = argv */
	tf->regs.gprs[1] = argc;
	tf->regs.gprs[2] = (uintptr_t) uargv;

	return 0;
}

int do_execve_arch_hook(int argc, char **kargv)
{
	return 0;
}

// kernel_execve - do SYS_exec syscall to exec a user program called by user_main kernel_thread
int kernel_execve(const char *name, const char **argv)
{
	int argc = 0;
	while (argv[argc] != NULL)
		argc++;

	uint32_t ret;
	register uint32_t _r3 asm("r3") = SYS_exec;
	register uint32_t _r4 asm("r4") = (uint32_t) name;
	register uint32_t _r5 asm("r5") = argc;
	register uint32_t _r6 asm("r6") = (uint32_t) argv;
	asm volatile ("l.sys 1; l.nop; l.nop;":"=r" (ret)
		      :"r"(_r3), "r"(_r4), "r"(_r5), "r"(_r6));

	return 0;
}

// cpu_idle - at the end of kern_init, the first kernel thread idleproc will do below works
void cpu_idle(void)
{
	while (1) {
		if (current->need_resched) {
			schedule();
		}
	}
}
