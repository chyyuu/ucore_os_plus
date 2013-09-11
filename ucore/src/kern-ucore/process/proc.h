#ifndef __KERN_PROCESS_PROC_H__
#define __KERN_PROCESS_PROC_H__

#include <types.h>
#include <list.h>
#include <trap.h>
#include <memlayout.h>
#include <unistd.h>
#include <sem.h>
#include <event.h>
#include <mp.h>
#include <elf.h>
#include <arch_proc.h>
#include <signal.h>
#include <spinlock.h>

// process's state in his life cycle
enum proc_state {
	PROC_UNINIT = 0,	// uninitialized
	PROC_SLEEPING,		// sleeping
	PROC_RUNNABLE,		// runnable(maybe running)
	PROC_ZOMBIE,		// almost dead, and wait parent proc to reclaim his resource
};

#define PROC_IS_IDLE(proc)   (((proc)->attribute & PROC_ATTR_ROLE) == PROC_ATTR_ROLE_IDLE)

#define PROC_NAME_LEN               15
#define MAX_PROCESS                 4096
#define MAX_PID                     (MAX_PROCESS * 2)

extern list_entry_t proc_list;
extern list_entry_t proc_mm_list;

struct inode;
struct fs_struct;

struct proc_struct {
	enum proc_state state;	// Process state
	int pid;		// Process ID
	int tid;
	int gid;
	int runs;		// the running times of Proces
	uintptr_t kstack;	// Process kernel stack
	volatile bool need_resched;	// bool value: need to be rescheduled to release CPU?
	struct proc_struct *parent;	// the parent process
	struct mm_struct *mm;	// Process's memory management field
	struct context context;	// Switch here to run process
	struct trapframe *tf;	// Trap frame for current interrupt
	uintptr_t cr3;		// CR3 register: the base addr of Page Directroy Table(PDT)
	uint32_t flags;		// Process flag
	char name[PROC_NAME_LEN + 1];	// Process name
	list_entry_t list_link;	// Process link list 
	list_entry_t hash_link;	// Process hash list
	int exit_code;		// return value when exit
	uint32_t wait_state;	// Process waiting state: the reason of sleeping
	struct proc_struct *cptr, *yptr, *optr;	// Process's children, yonger sibling, Old sibling
	list_entry_t thread_group;	// the threads list including this proc which share resource (mem/file/sem...)

	struct arch_proc_struct arch;	// Arch dependant info. See arch_proc.h

	struct run_queue *rq;	// running queue contains Process
	list_entry_t run_link;	// the entry linked in run queue
	int time_slice;		// time slice for occupying the CPU
	sem_queue_t *sem_queue;	// the user semaphore queue which process waits
	event_t event_box;	// the event which process waits   
	struct fs_struct *fs_struct;	// the file related info(pwd, files_count, files_array, fs_semaphore) of process

	struct proc_signal signal_info;

	void *tls_pointer;

	int cpu_affinity;
	spinlock_s lock;
};

#define PROC_CPU_NO_AFFINITY (-1)
#define set_proc_cpu_affinity(proc, cpuid) \
	do{(proc)->cpu_affinity = cpuid;}while(0)

struct linux_timespec {
	long tv_sec;		/* seconds */
	long tv_nsec;		/* nanoseconds */
};

#define PF_EXITING                  0x00000001	// getting shutdown
#define PF_PINCPU                   0x00000002

//the wait state
#define WT_CHILD                    (0x00000001 | WT_INTERRUPTED)	// wait child process
#define WT_TIMER                    (0x00000002 | WT_INTERRUPTED)	// wait timer
#define WT_KSWAPD                    0x00000003	// wait kswapd to free page
#define WT_KBD                      (0x00000004 | WT_INTERRUPTED)	// wait the input of keyboard
#define WT_KSEM                      0x00000100	// wait kernel semaphore
#define WT_USEM                     (0x00000101 | WT_INTERRUPTED)	// wait user semaphore
#define WT_EVENT_SEND               (0x00000110 | WT_INTERRUPTED)	// wait the sending event
#define WT_EVENT_RECV               (0x00000111 | WT_INTERRUPTED)	// wait the recving event
#define WT_MBOX_SEND                (0x00000120 | WT_INTERRUPTED)	// wait the sending mbox
#define WT_MBOX_RECV                (0x00000121 | WT_INTERRUPTED)	// wait the recving mbox
#define WT_PIPE                     (0x00000200 | WT_INTERRUPTED)	// wait the pipe
#define WT_SIGNAL					          (0x00000400 | WT_INTERRUPTED)	// wait the signal
#define WT_KERNEL_SIGNAL            (0x00000800| WT_INTERRUPTED)
#define WT_INTERRUPTED               0x80000000	// the wait state could be interrupted

#define TIF_SIGPENDING				0x00010000

#define le2proc(le, member)         \
  to_struct((le), struct proc_struct, member)

#define current (mycpu()->__current)
#define idleproc (mycpu()->idleproc)

extern struct proc_struct *initproc;
extern struct proc_struct *kswapd;

void proc_init(void);
void proc_init_ap(void);

void proc_run(struct proc_struct *proc);
int ucore_kernel_thread(int (*fn) (void *), void *arg, uint32_t clone_flags);

char *set_proc_name(struct proc_struct *proc, const char *name);
char *get_proc_name(struct proc_struct *proc);
void cpu_idle(void) __attribute__ ((noreturn));
void kernel_thread_entry(void);

struct proc_struct *find_proc(int pid);
void may_killed(void);
int do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf);
int do_exit(int error_code);
int do_exit_thread(int error_code);
//int do_execve(const char *name, int argc, const char **argv);
int do_execve(const char *name, const char **argv, const char **envp);
int do_yield(void);
int do_wait(int pid, int *code_store);
int do_kill(int pid, int error_code);
int do_brk(uintptr_t * brk_store);
int do_linux_brk(uintptr_t brk);
int do_sleep(unsigned int time);
int do_mmap(uintptr_t * addr_store, size_t len, uint32_t mmap_flags);
int do_munmap(uintptr_t addr, size_t len);
int do_shmem(uintptr_t * addr_store, size_t len, uint32_t mmap_flags);
int do_linux_waitpid(int pid, int *code_store);

/* Implemented by archs */
struct proc_struct *alloc_proc(void);
struct proc_struct *next_thread(struct proc_struct *proc);
void switch_to(struct context *from, struct context *to);

/* For TLS(Thread Local Storage */
void tls_switch(struct proc_struct *proc);

void de_thread_arch_hook(struct proc_struct *proc);
int copy_thread(uint32_t clone_flags, struct proc_struct *proc,
		uintptr_t user_stack, struct trapframe *tf);
int init_new_context(struct proc_struct *proc, struct elfhdr *elf,
		     int argc, char **kargv, int envc, char **kenvp);
#ifdef UCONFIG_BIONIC_LIBC
int init_new_context_dynamic(struct proc_struct *proc, struct elfhdr *elf,
			     int argc, char **kargv, int envc, char **kenvp,
			     uint32_t is_dynamic, uint32_t real_entry,
			     uint32_t load_address, uint32_t linker_base);
#endif //UCONFIG_BIONIC_LIBC

int kernel_thread(int (*fn) (void *), void *arg, uint32_t clone_flags);
int kernel_execve(const char *name, const char **argv, const char **kenvp);
int do_execve_arch_hook(int argc, char **kargv);

#endif /* !__KERN_PROCESS_PROC_H__ */
