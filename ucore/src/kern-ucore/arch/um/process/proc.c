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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <arch.h>
#include <stub.h>
#include <host_syscall.h>
#include <swap.h>
#include <fs.h>
#include <kio.h>

#define current (pls_read(current))

/**
 * Switch to another context.
 *     Note that context switching actually meas thread switching in umUcore.
 *         (see the internal doc for details)
 * @param from the original context
 * @param to the target context
 */
void switch_to(struct context *from, struct context *to)
{
	if (setjmp(from->switch_buf) == 0)
		longjmp(to->switch_buf, 1);
}

/**
 * Allocate a proc_struct and initialize its fields.
 *     Most of the function is same with that in i386. It's here just for avoiding another hook...
 * @return the proc_struct created, or NULL if failed.
 */
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
		memset(proc->name, 0, PROC_NAME_LEN);
		proc->wait_state = 0;
		proc->cptr = proc->optr = proc->yptr = NULL;
		list_init(&(proc->thread_group));

		memset(&(proc->context), 0, sizeof(struct context));
		proc->tf = NULL;
		proc->flags = 0;

		proc->rq = NULL;
		list_init(&(proc->run_link));
		proc->time_slice = 0;

		/* These are arch-dependent parts. */
		proc->arch.host = NULL;
		proc->arch.forking = 0;
		proc->sem_queue = NULL;
		event_box_init(&(proc->event_box));
		proc->fs_struct = NULL;
	}
	return proc;
}

/**
 * Create a kernel thread and let it run @fn, giving the arguments @arg
 *     the trapframe created is just an adapter to the existing function prototype.
 * @param fn the function the created thread is going to execute
 * @param arg the arguments for the thread
 * @param clone_flags flags that control what do_fork do, see include/unistd.h for details.
 * @return 0 if succeeds, or a negative otherwise.
 */
int kernel_thread(int (*fn) (void *), void *arg, uint32_t clone_flags)
{
	struct trapframe tf;
	memset(&tf, 0, sizeof(struct trapframe));
	tf.fn = fn;
	tf.arg = arg;
	return do_fork(clone_flags | CLONE_VM, 0, &tf);
}

/**
 * The hook called everytime 'de_thread' is called to perform arch-related operations.
 *     In umUcore, what should be done is checking whether the container process contains nothing
 *     and release it if it does.
 * Note: For do_execve, a container may be destroyed and another be created.
 *       We should use the old one directly in the future.
 * @param proc the PCB of the process to be dettached
 */
void de_thread_arch_hook(struct proc_struct *proc)
{
	if (proc->arch.host != NULL) {
		assert(proc->arch.host->nr_threads > 0);
		proc->arch.host->nr_threads--;
		if (proc->arch.host->nr_threads == 0) {
			free_page(kva2page(current->arch.host->stub_stack));
			kill_ptraced_process(current->arch.host->host_pid, 1);
			kfree(current->arch.host);
			current->arch.host = NULL;
		}
	}
}

/**
 * Make a copy of the current thread/process, giving the parent the child's pid and the child 0.
 *     This is called in do_fork after all structures in the child's PCB are ready.
 * @param clone_flags we need this to determine whether we're creating a 'thread' or a 'process'
 * @param proc the PCB of the child
 * @param user_stack the stack used by the child
 * @param tf the struct containing 'fn' and 'arg'
 * @return 0 if succeeds, or -1 otherwise
 */
int
copy_thread(uint32_t clone_flags, struct proc_struct *proc,
	    uintptr_t user_stack, struct trapframe *tf)
{
	int pid;
	void *stub_stack;

	/* If 'do_fork' is called by the kernel, 'current' should be idle,
	 *     and we're actually creating a kernel thread .
	 * If 'do_fork' is called by the user (i.e. syscall), 'current' is a user PCB,
	 *     and we need to create another user process.
	 */
	if (RUN_AS_USER) {
		if (clone_flags & CLONE_VM) {
			/* Use current host process as its container */
			proc->arch.host = current->arch.host;
			proc->arch.host->nr_threads++;
		} else {
			/* Create a new child process */
			proc->arch.host =
			    kmalloc(sizeof(struct host_container));
			if (proc->arch.host == NULL)
				goto exit;

			stub_stack = boot_alloc_page();
			if (stub_stack == NULL)
				goto exit_free_host;
			pid = start_userspace(stub_stack);

			if (pid < 0)
				goto exit_free_stub;

			/* Initialize host process descriptor */
			proc->arch.forking = 0;
			proc->arch.host->host_pid = pid;
			proc->arch.host->nr_threads = 1;
			proc->arch.host->stub_stack =
			    (struct stub_stack *)stub_stack;
			/* unmap kernel area. */
			if (host_munmap
			    (proc, (void *)KERNBASE, KERNTOP - KERNBASE) < 0)
				panic("unmap kernel area failed \n");
		}
		/* The child should have the same regs as the parent's. */
		memcpy(&(proc->arch.regs.regs), &(current->arch.regs.regs),
		       sizeof(struct user_regs_struct));

		/* make the child return 0 for the syscall */
		proc->arch.regs.regs.eax = 0;

		proc->arch.regs.regs.esp = user_stack;

		/* The current thread will run in 'userspace' */
		proc->context.switch_buf->__ebx = (uint32_t) userspace;
		proc->context.switch_buf->__ebp =
		    (uint32_t) & (proc->arch.regs);
	} else {
		/* For kernel thread */
		proc->context.switch_buf->__ebx = (uint32_t) (tf->fn);
		proc->context.switch_buf->__ebp = (uint32_t) (tf->arg);
	}
	/* All new threads/processes start running from 'kernel_thread_entry'
	 *     for the 'processes' actually means 'monitor threads' to the kernel.
	 */
	proc->context.switch_buf->__eip = (uint32_t) kernel_thread_entry;
	proc->context.switch_buf->__esp = proc->kstack + KSTACKSIZE;

	return 0;

exit_free_stub:
	free_page(kva2page(stub_stack));
exit_free_host:
	kfree(proc->arch.host);
exit:
	return -1;
}

/**
 * Initialize the context for a process created by 'do_execve'.
 * @param proc the PCB of the process created
 * @param elf the executable file loaded whose entry is necessary for initializing
 * @param argc the number of arguments
 * @param kargv the actually arguments copy in the kernel
 * @return 0 (always succeeds)
 */
int
init_new_context(struct proc_struct *proc, struct elfhdr *elf, int argc,
		 char **kargv)
{
	uintptr_t stacktop = USTACKTOP - argc * PGSIZE;
	char **uargv = (char **)(stacktop - argc * sizeof(char *));
	stacktop = (uintptr_t) uargv - sizeof(int);

	proc->arch.regs.regs.esp = stacktop;
	proc->arch.regs.regs.eip = elf->e_entry;
	proc->arch.regs.regs.xcs = 0x73;
	proc->arch.regs.regs.xds = proc->arch.regs.regs.xes =
	    proc->arch.regs.regs.xfs = proc->arch.regs.regs.xgs =
	    proc->arch.regs.regs.xss = 0x7b;

	return 0;
}

/**
 * The hook called before 'do_execve' successfully return.
 *     What we need to do here are:
 *       1. create a new host container if the process doesn't have one yet;
 *       2. create the stack for syscall stub;
 *       3. unmap umUcore kernel area in the child and erasing the info in the page table;
 *       4. copy arguments to the user stack of the child, and free the kernel's copy;
 *       5. call 'userspace'.
 *     If everything is done, the current thread becomes a monitor thread forever.
 * @param argc the number of arguments
 * @param kargv the copy of the arguments in the kernel
 */
int do_execve_arch_hook(int argc, char **kargv)
{
	if (current->arch.host == NULL) {
		current->arch.host = kmalloc(sizeof(struct host_container));
		if (current->arch.host == NULL)
			return -1;
	}

	void *stub_stack = boot_alloc_page();
	if (stub_stack == NULL)
		goto free_host;
	int ret = start_userspace(stub_stack);
	if (ret <= 0)
		goto free_stub_stack;

	current->arch.host->stub_stack = stub_stack;
	current->arch.host->host_pid = ret;
	current->arch.host->nr_threads = 1;

	/* unmap kernel area */
	if (host_munmap(current, (void *)KERNBASE, KERNTOP - KERNBASE) < 0)
		panic("unmap kernel area failed\n");

	/* erase kernel maps in the page table */
	int valid_size = KERNBASE / PTSIZE * sizeof(pte_t);
	memset((void *)((int)(current->mm->pgdir) + valid_size),
	       0, PGSIZE - valid_size);

	/* Copy arguments */
	current->arch.regs.is_user = 1;
	uintptr_t stacktop = USTACKTOP - argc * PGSIZE;
	char **uargv = (char **)(stacktop - argc * sizeof(char *));
	int i, addr;
	for (i = 0; i < argc; i++) {
		addr = stacktop + i * PGSIZE;
		assert(copy_to_user
		       (current->mm, uargv + i, &addr, sizeof(int)));
		assert(copy_to_user
		       (current->mm, (void *)addr, kargv[i],
			strlen(kargv[i]) + 1));
	}
	stacktop = (uintptr_t) uargv - sizeof(int);
	copy_to_user(current->mm, (void *)stacktop, &argc, sizeof(int));

	/* The copy of the args in the kernel will never be used again and we will not return,
	 *     so free them here.
	 */
	while (argc > 0) {
		kfree(kargv[--argc]);
	}
	userspace(&(current->arch.regs));
	/* should never comes here */

free_stub_stack:
	free_page(kva2page(stub_stack));
free_host:
	kfree(current->arch.host);
	current->arch.host = NULL;
	return -1;
}

/**
 * Exec a user program called by user_main kernel_thread
 * Note: we don't need to emulate an interrupt here.
 * @param name the name of the process
 * @param argv the arguments
 */
int kernel_execve(const char *name, const char **argv)
{
	int argc = 0, ret;
	while (argv[argc] != NULL) {
		argc++;
	}
	ret = do_execve(name, argc, argv);
	kprintf("error with code: %d\n", ret);
	return ret;
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
