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
#include <unistd.h>
#include <fs.h>
#include <vfs.h>
#include <sysfile.h>

/* ------------- process/thread mechanism design&implementation -------------
(an simplified Linux process/thread mechanism )
introduction:
  ucore implements a simple process/thread mechanism. process contains the independent memory sapce, at least one threads
for execution, the kernel data(for management), processor state (for context switch), files(in lab6), etc. ucore needs to
manage all these details efficiently. In ucore, a thread is just a special kind of process(share process's memory).
------------------------------
process state       :     meaning               -- reason
    PROC_UNINIT     :   uninitialized           -- alloc_proc
    PROC_SLEEPING   :   sleeping                -- try_free_pages, do_wait, do_sleep
    PROC_RUNNABLE   :   runnable(maybe running) -- proc_init, wakeup_proc, 
    PROC_ZOMBIE     :   almost dead             -- do_exit

-----------------------------
process state changing:
                                            
  alloc_proc                                 RUNNING
      +                                   +--<----<--+
      +                                   + proc_run +
      V                                   +-->---->--+ 
PROC_UNINIT -- proc_init/wakeup_proc --> PROC_RUNNABLE -- try_free_pages/do_wait/do_sleep --> PROC_SLEEPING --
                                           A      +                                                           +
                                           |      +--- do_exit --> PROC_ZOMBIE                                +
                                           +                                                                  + 
                                           -----------------------wakeup_proc----------------------------------
-----------------------------
process relations
parent:           proc->parent  (proc is children)
children:         proc->cptr    (proc is parent)
older sibling:    proc->optr    (proc is younger sibling)
younger sibling:  proc->yptr    (proc is older sibling)
-----------------------------
related syscall for process:
SYS_exit        : process exit,                           -->do_exit
SYS_fork        : create child process, dup mm            -->do_fork-->wakeup_proc
SYS_wait        : wait process                            -->do_wait
SYS_exec        : after fork, process execute a program   -->load a program and refresh the mm
SYS_clone       : create child thread                     -->do_fork-->wakeup_proc
SYS_yield       : process flag itself need resecheduling, -- proc->need_sched=1, then scheduler will rescheule this process
SYS_sleep       : process sleep                           -->do_sleep 
SYS_kill        : kill process                            -->do_kill-->proc->flags |= PF_EXITING
                                                                 -->wakeup_proc-->do_wait-->do_exit   
SYS_getpid      : get the process's pid

*/

// the process set's list
list_entry_t proc_list;

#define HASH_SHIFT          10
#define HASH_LIST_SIZE      (1 << HASH_SHIFT)
#define pid_hashfn(x)       (hash32(x, HASH_SHIFT))

// has list for process set based on pid
static list_entry_t hash_list[HASH_LIST_SIZE];

#define current (pls_read(current))

static int nr_process = 0;

void kernel_thread_entry(void);
void forkrets(struct trapframe *tf);
void switch_to(struct context *from, struct context *to);

// alloc_proc - alloc a proc_struct and init all fields of proc_struct
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
		proc->tf = NULL;
		proc->flags = 0;
		proc->need_resched = 0;
		proc->cr3 = boot_cr3;
		memset(&(proc->context), 0, sizeof(struct context));
		memset(proc->name, 0, PROC_NAME_LEN);
		proc->exit_code = 0;
		proc->wait_state = 0;
		list_init(&(proc->run_link));
		list_init(&(proc->list_link));
		proc->time_slice = 0;
		proc->cptr = proc->yptr = proc->optr = NULL;
		event_box_init(&(proc->event_box));
		proc->fs_struct = NULL;
		proc->sem_queue = sem_queue_create();
	}
	return proc;
}

// set_links - set the relation links of process
static void set_links(struct proc_struct *proc)
{
	list_add(&proc_list, &(proc->list_link));
	proc->yptr = NULL;
	if ((proc->optr = proc->parent->cptr) != NULL) {
		proc->optr->yptr = proc;
	}
	proc->parent->cptr = proc;
	nr_process++;
}

// remove_links - clean the relation links of process
static void remove_links(struct proc_struct *proc)
{
	list_del(&(proc->list_link));
	if (proc->optr != NULL) {
		proc->optr->yptr = proc->yptr;
	}
	if (proc->yptr != NULL) {
		proc->yptr->optr = proc->optr;
	} else {
		proc->parent->cptr = proc->optr;
	}
	nr_process--;
}

// get_pid - alloc a unique pid for process
static int get_pid(void)
{
	static_assert(MAX_PID > MAX_PROCESS);
	struct proc_struct *proc;
	list_entry_t *list = &proc_list, *le;
	static int next_safe = MAX_PID, last_pid = MAX_PID;
	if (++last_pid >= MAX_PID) {
		last_pid = 1;
		goto inside;
	}
	if (last_pid >= next_safe) {
inside:
		next_safe = MAX_PID;
repeat:
		le = list;
		while ((le = list_next(le)) != list) {
			proc = le2proc(le, list_link);
			if (proc->pid == last_pid) {
				if (++last_pid >= next_safe) {
					if (last_pid >= MAX_PID) {
						last_pid = 1;
					}
					next_safe = MAX_PID;
					goto repeat;
				}
			} else if (proc->pid > last_pid
				   && next_safe > proc->pid) {
				next_safe = proc->pid;
			}
		}
	}
	return last_pid;
}

// forkret -- the first kernel entry point of a new thread/process
// NOTE: the addr of forkret is setted in copy_thread function
//       after switch_to, the current proc will execute here.
static void forkret(void)
{				//kprintf(" [forkrets pid=%d epc=%x] ", current->pid, current->tf->tf_epc);
/*    kprintf("{");
    int i;
    for (i = 0; i < 10; ++i) kprintf("%x:%x ", (current->tf->tf_epc + i * 4), *(uint32_t*)(current->tf->tf_epc + i * 4));
    kprintf("} ");*/
	forkrets(current->tf);
}

// hash_proc - add proc into proc hash_list
static void hash_proc(struct proc_struct *proc)
{
	list_add(hash_list + pid_hashfn(proc->pid), &(proc->hash_link));
}

// unhash_proc - delete proc from proc hash_list
static void unhash_proc(struct proc_struct *proc)
{
	list_del(&(proc->hash_link));
}

// kernel_thread - create a kernel thread using "fn" function
// NOTE: the contents of temp trapframe tf will be copied to 
//       proc->tf in do_fork-->copy_thread function
int kernel_thread(int (*fn) (void *), void *arg, uint32_t clone_flags)
{
	struct trapframe tf;
	memset(&tf, 0, sizeof(struct trapframe));
	tf.tf_regs.reg_r[MIPS_REG_A0] = (uint32_t) arg;
	tf.tf_regs.reg_r[MIPS_REG_A1] = (uint32_t) fn;
	tf.tf_regs.reg_r[MIPS_REG_V0] = 0;
	//TODO
	tf.tf_status = read_c0_status();
	tf.tf_status &= ~ST0_KSU;
	tf.tf_status |= ST0_IE;
	tf.tf_status |= ST0_EXL;
	tf.tf_regs.reg_r[MIPS_REG_GP] = __read_reg($28);
	tf.tf_epc = (uint32_t) kernel_thread_entry;
	return do_fork(clone_flags | CLONE_VM, 0, &tf);
}

// setup_kstack - alloc pages with size KSTACKPAGE as process kernel stack
static int setup_kstack(struct proc_struct *proc)
{
	struct Page *page = alloc_pages(KSTACKPAGE);
	if (page != NULL) {
		proc->kstack = (uintptr_t) page2kva(page);
		return 0;
	}
	return -E_NO_MEM;
}

// put_kstack - free the memory space of process kernel stack
static void put_kstack(struct proc_struct *proc)
{
	free_pages(kva2page((void *)(proc->kstack)), KSTACKPAGE);
}

// setup_pgdir - alloc one page as PDT
static int setup_pgdir(struct mm_struct *mm)
{
	struct Page *page;
	if ((page = alloc_page()) == NULL) {
		return -E_NO_MEM;
	}
	pde_t *pgdir = page2kva(page);
	memcpy(pgdir, boot_pgdir, PGSIZE);
	//panic("unimpl");
	//pgdir[PDX(VPT)] = PADDR(pgdir) | PTE_P | PTE_W;
	mm->pgdir = pgdir;
	return 0;
}

// put_pgdir - free the memory space of PDT
static void put_pgdir(struct mm_struct *mm)
{
	free_page(kva2page(mm->pgdir));
}

// copy_mm - process "proc" duplicate OR share process "current"'s mm according clone_flags
//         - if clone_flags & CLONE_VM, then "share" ; else "duplicate"
static int copy_mm(uint32_t clone_flags, struct proc_struct *proc)
{
	struct mm_struct *mm, *oldmm = current->mm;

	/* current is a kernel thread */
	if (oldmm == NULL) {
		return 0;
	}
	if (clone_flags & CLONE_VM) {
		mm = oldmm;
		goto good_mm;
	}

	int ret = -E_NO_MEM;
	if ((mm = mm_create()) == NULL) {
		goto bad_mm;
	}
	if (setup_pgdir(mm) != 0) {
		goto bad_pgdir_cleanup_mm;
	}

	lock_mm(oldmm);
	{
		ret = dup_mmap(mm, oldmm);
	}
	unlock_mm(oldmm);

	if (ret != 0) {
		goto bad_dup_cleanup_mmap;
	}

good_mm:
	mm_count_inc(mm);
	proc->mm = mm;
	proc->cr3 = PADDR(mm->pgdir);
	return 0;
bad_dup_cleanup_mmap:
	exit_mmap(mm);
	put_pgdir(mm);
bad_pgdir_cleanup_mm:
	mm_destroy(mm);
bad_mm:
	return ret;
}

// copy_thread - setup the trapframe on the  process's kernel stack top and
//             - setup the kernel entry point and stack of process
int
copy_thread(uint32_t clone_flags, struct proc_struct *proc, uintptr_t esp,
	    struct trapframe *tf)
{
	proc->tf = (struct trapframe *)(proc->kstack + KSTACKSIZE) - 1;
	*(proc->tf) = *tf;
	proc->tf->tf_regs.reg_r[MIPS_REG_V0] = 0;
	if (esp == 0)		//a kernel thread
		esp = (uintptr_t) proc->tf - 32;
	proc->tf->tf_regs.reg_r[MIPS_REG_SP] = esp;
	proc->context.sf_ra = (uintptr_t) forkret;
	proc->context.sf_sp = (uintptr_t) (proc->tf) - 32;
	return 0;
}

//copy_fs&put_fs function used by do_fork in LAB8
static int copy_fs(uint32_t clone_flags, struct proc_struct *proc)
{
	struct fs_struct *fs_struct, *old_fs_struct = current->fs_struct;
	assert(old_fs_struct != NULL);

	if (clone_flags & CLONE_FS) {
		fs_struct = old_fs_struct;
		goto good_fs_struct;
	}

	int ret = -E_NO_MEM;
	if ((fs_struct = fs_create()) == NULL) {
		goto bad_fs_struct;
	}

	if ((ret = dup_fs(fs_struct, old_fs_struct)) != 0) {
		goto bad_dup_cleanup_fs;
	}

good_fs_struct:
	fs_count_inc(fs_struct);
	proc->fs_struct = fs_struct;
	return 0;

bad_dup_cleanup_fs:
	fs_destroy(fs_struct);
bad_fs_struct:
	return ret;
}

static void put_fs(struct proc_struct *proc)
{
	struct fs_struct *fs_struct = proc->fs_struct;
	if (fs_struct != NULL) {
		if (fs_count_dec(fs_struct) == 0) {
			fs_destroy(fs_struct);
		}
	}
}

// this function isn't very correct in LAB8
static void put_kargv(int argc, char **kargv)
{
	while (argc > 0) {
		kfree(kargv[--argc]);
	}
}

static int
copy_kargv(struct mm_struct *mm, int argc, char **kargv, const char **argv)
{
	int i, ret = -E_INVAL;
	if (!user_mem_check
	    (mm, (uintptr_t) argv, sizeof(const char *) * argc, 0)) {
		return ret;
	}
	for (i = 0; i < argc; i++) {
		char *buffer;
		if ((buffer = kmalloc(EXEC_MAX_ARG_LEN + 1)) == NULL) {
			goto failed_nomem;
		}
		if (!copy_string(mm, buffer, argv[i], EXEC_MAX_ARG_LEN + 1)) {
			kfree(buffer);
			goto failed_cleanup;
		}
		kargv[i] = buffer;
	}
	return 0;

failed_nomem:
	ret = -E_NO_MEM;
failed_cleanup:
	put_kargv(i, kargv);
	return ret;
}

// kernel_execve - do SYS_exec syscall to exec a user program scalled by user_main kernel_thread
int kernel_execve(const char *name, const char **argv, const char **kenvp)
{
	int argc = 0, ret;

	while (argv[argc] != NULL) {
		argc++;
	}
	//panic("unimpl");
	asm volatile ("la $v0, %1;\n"	/* syscall no. */
		      "move $a0, %2;\n"
		      "move $a1, %3;\n"
		      "move $a2, %4;\n"
		      "move $a3, %5;\n"
		      "syscall;\n" "nop;\n" "move %0, $v0;\n":"=r" (ret)
		      :"i"( /*T_SYSCALL+ */ SYS_exec), "r"(name), "r"(argv),
		      "r"(kenvp), "r"(argc)
		      :"a0", "a1", "a2", "a3", "v0");
	return ret;
}

#define __KERNEL_EXECVE(name, path, ...) ({                         \
const char *argv[] = {path, ##__VA_ARGS__, NULL};       \
					 kprintf("kernel_execve: pid = %d, name = \"%s\".\n",    \
							 current->pid, name);                            \
					 kernel_execve(name, argv);                              \
})

#define KERNEL_EXECVE(x, ...)                   __KERNEL_EXECVE(#x, #x, ##__VA_ARGS__)

#define KERNEL_EXECVE2(x, ...)                  KERNEL_EXECVE(x, ##__VA_ARGS__)

#define __KERNEL_EXECVE3(x, s, ...)             KERNEL_EXECVE(x, #s, ##__VA_ARGS__)

#define KERNEL_EXECVE3(x, s, ...)               __KERNEL_EXECVE3(x, s, ##__VA_ARGS__)

// cpu_idle - at the end of kern_init, the first kernel thread idleproc will do below works
void cpu_idle(void)
{
	while (1) {
		if (current->need_resched) {
			schedule();
		}
	}
}

//added by HHL
int ucore_kernel_thread(int (*fn) (void *), void *arg, uint32_t clone_flags)
{
	kernel_thread(fn, arg, clone_flags);
}

void de_thread_arch_hook(struct proc_struct *proc)
{
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
	//stacktop = (uintptr_t)uargv - sizeof(int);
	//*(int *)stacktop = argc;
	struct trapframe *tf = proc->tf;
	memset(tf, 0, sizeof(struct trapframe));
	tf->tf_epc = elf->e_entry;
	tf->tf_regs.reg_r[MIPS_REG_SP] = USTACKTOP;
	uint32_t status = read_c0_status();
	status &= ~ST0_KSU;
	status |= KSU_USER;
	status |= ST0_EXL;
	tf->tf_status = status;
	tf->tf_regs.reg_r[MIPS_REG_A0] = argc;
	tf->tf_regs.reg_r[MIPS_REG_A1] = (uint32_t) uargv;

	return 0;
}

int do_execve_arch_hook(int argc, char **kargv)
{
	return 0;
}
