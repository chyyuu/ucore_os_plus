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
#include <sched.h>
#include <stdlib.h>
#include <assert.h>
#include <elf.h>
#include <fs.h>
#include <vfs.h>
#include <sysfile.h>
#include <swap.h>
#include <mbox.h>
#include <kio.h>
#include <stdio.h>
#include <mp.h>
#include <resource.h>
#include <sysconf.h>
#include <refcache.h>
#include <spinlock.h>

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

struct proc_struct *initproc;
#ifdef UCONFIG_SWAP
struct proc_struct *kswapd;
#endif

/* lock to protect the following data struct */
spinlock_s proc_lock;
// the process set's list
list_entry_t proc_list;
// the process set's mm's list
list_entry_t proc_mm_list;

#define HASH_SHIFT          10
#define HASH_LIST_SIZE      (1 << HASH_SHIFT)
#define pid_hashfn(x)       (hash32(x, HASH_SHIFT))

// has list for process set based on pid
static list_entry_t hash_list[HASH_LIST_SIZE];
static int nr_process = 0;

////////////////////////////////////////////////

static int __do_exit(void);
static int __do_kill(struct proc_struct *proc, int error_code);

// set_proc_name - set the name of proc
char *set_proc_name(struct proc_struct *proc, const char *name)
{
	memset(proc->name, 0, sizeof(proc->name));
	return memcpy(proc->name, name, PROC_NAME_LEN);
}

#if 0
// get_proc_name - get the name of proc
char *get_proc_name(struct proc_struct *proc)
{
	static char name[PROC_NAME_LEN + 1];
	memset(name, 0, sizeof(name));
	return memcpy(name, proc->name, PROC_NAME_LEN);
}
#endif

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
		last_pid = sysconf.lcpu_count;
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

// proc_run - make process "proc" running on cpu
// NOTE: before call switch_to, should load  base addr of "proc"'s new PDT
void proc_run(struct proc_struct *proc)
{
	if (proc != current) {
		bool intr_flag;
		struct proc_struct *prev = current, *next = proc;
		// kprintf("(%d) => %d\n", lapic_id, next->pid);
		local_intr_save(intr_flag);
		{
			current = proc;
			load_rsp0(next->kstack + KSTACKSIZE);
			mp_set_mm_pagetable(next->mm);

#ifdef UCONFIG_BIONIC_LIBC
			// for tls switch
			tls_switch(next);
#endif //UCONFIG_BIONIC_LIBC
			switch_to(&(prev->context), &(next->context));
		}
		local_intr_restore(intr_flag);
	}
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

// find_proc - find proc frome proc hash_list according to pid
struct proc_struct *find_proc(int pid)
{
	if (0 < pid && pid < MAX_PID) {
		list_entry_t *list = hash_list + pid_hashfn(pid), *le = list;
		while ((le = list_next(le)) != list) {
			struct proc_struct *proc = le2proc(le, hash_link);
			if (proc->pid == pid) {
				return proc;
			}
		}
	}
	return NULL;
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
// XXX, PDT size != PGSIZE!!!
#ifndef ARCH_ARM
static int setup_pgdir(struct mm_struct *mm)
{
	struct Page *page;
	if ((page = alloc_page()) == NULL) {
		return -E_NO_MEM;
	}
	pgd_t *pgdir = page2kva(page);
	memcpy(pgdir, init_pgdir_get(), PGSIZE);
	map_pgdir(pgdir);
	mm->pgdir = pgdir;
	return 0;
}

// put_pgdir - free the memory space of PDT
static void put_pgdir(struct mm_struct *mm)
{
	free_page(kva2page(mm->pgdir));
}
#else
/* ARM PDT is 16k */
static int setup_pgdir(struct mm_struct *mm)
{
	struct Page *page;
	/* 4 * 4K = 16K */
	/* dirty hack */
	if ((page = alloc_pages(8)) == NULL) {
		return -E_NO_MEM;
	}
	pgd_t *pgdir_start = page2kva(page);
	pgd_t *pgdir = ROUNDUP(pgdir_start, 4 * PGSIZE);
	memcpy(pgdir, init_pgdir_get(), 4 * PGSIZE);

	map_pgdir(pgdir);
	mm->pgdir = pgdir;
	mm->pgdir_alloc_addr = pgdir_start;
	return 0;
}

// put_pgdir - free the memory space of PDT
static void put_pgdir(struct mm_struct *mm)
{
	assert(mm->pgdir_alloc_addr);
	free_pages(kva2page(mm->pgdir_alloc_addr), 8);
}
#endif

// de_thread - delete this thread "proc" from thread_group list
static void de_thread(struct proc_struct *proc)
{
	if (!list_empty(&(proc->thread_group))) {
		bool intr_flag;
		local_intr_save(intr_flag);
		{
			list_del_init(&(proc->thread_group));
		}
		local_intr_restore(intr_flag);
	}

	de_thread_arch_hook(proc);
}

// next_thread - get the next thread "proc" from thread_group list
struct proc_struct *next_thread(struct proc_struct *proc)
{
	return le2proc(list_next(&(proc->thread_group)), thread_group);
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

#ifdef UCONFIG_BIONIC_LIBC
	lock_mm(mm);
	{
		ret = remapfile(mm, proc);
	}
	unlock_mm(mm);
#endif //UCONFIG_BIONIC_LIBC

	if (ret != 0) {
		goto bad_dup_cleanup_mmap;
	}

good_mm:
	if (mm != oldmm) {
		mm->brk_start = oldmm->brk_start;
		mm->brk = oldmm->brk;
		bool intr_flag;
		local_intr_save(intr_flag);
		{
			list_add(&(proc_mm_list), &(mm->proc_mm_link));
		}
		local_intr_restore(intr_flag);
	}
	mm_count_inc(mm);
	proc->mm = mm;
	set_pgdir(proc, mm->pgdir);
	return 0;
bad_dup_cleanup_mmap:
	exit_mmap(mm);
	put_pgdir(mm);
bad_pgdir_cleanup_mm:
	mm_destroy(mm);
bad_mm:
	return ret;
}

static int copy_sem(uint32_t clone_flags, struct proc_struct *proc)
{
	sem_queue_t *sem_queue, *old_sem_queue = current->sem_queue;

	/* current is kernel thread */
	if (old_sem_queue == NULL) {
		return 0;
	}

	if (clone_flags & CLONE_SEM) {
		sem_queue = old_sem_queue;
		goto good_sem_queue;
	}

	int ret = -E_NO_MEM;
	if ((sem_queue = sem_queue_create()) == NULL) {
		goto bad_sem_queue;
	}

	down(&(old_sem_queue->sem));
	ret = dup_sem_queue(sem_queue, old_sem_queue);
	up(&(old_sem_queue->sem));

	if (ret != 0) {
		goto bad_dup_cleanup_sem;
	}

good_sem_queue:
	sem_queue_count_inc(sem_queue);
	proc->sem_queue = sem_queue;
	return 0;

bad_dup_cleanup_sem:
	exit_sem_queue(sem_queue);
	sem_queue_destroy(sem_queue);
bad_sem_queue:
	return ret;
}

static void put_sem_queue(struct proc_struct *proc)
{
	sem_queue_t *sem_queue = proc->sem_queue;
	if (sem_queue != NULL) {
		if (sem_queue_count_dec(sem_queue) == 0) {
			exit_sem_queue(sem_queue);
			sem_queue_destroy(sem_queue);
		}
	}
}

static int copy_signal(uint32_t clone_flags, struct proc_struct *proc)
{
	struct signal_struct *signal, *oldsig = current->signal_info.signal;

	if (oldsig == NULL) {
		return 0;
	}
#if 0
	if (clone_flags & CLONE_THREAD) {
		signal = oldsig;
		goto good_signal;
	}
#endif

	int ret = -E_NO_MEM;
	if ((signal = signal_create()) == NULL) {
		goto bad_signal;
	}

good_signal:
	signal_count_inc(signal);
	proc->signal_info.signal = signal;
	return 0;

bad_signal:
	return ret;
}

// copy_thread - setup the trapframe on the  process's kernel stack top and
//             - setup the kernel entry point and stack of process
static void put_signal(struct proc_struct *proc)
{
	struct signal_struct *sig = proc->signal_info.signal;
	if (sig != NULL) {
		if (signal_count_dec(sig) == 0) {
			signal_destroy(sig);
		}
	}
	proc->signal_info.signal = NULL;
}

static int copy_sighand(uint32_t clone_flags, struct proc_struct *proc)
{
	struct sighand_struct *sighand, *oldsh = current->signal_info.sighand;

	if (oldsh == NULL) {
		return 0;
	}

	if (clone_flags & (CLONE_SIGHAND | CLONE_THREAD)) {
		sighand = oldsh;
		goto good_sighand;
	}

	int ret = -E_NO_MEM;
	if ((sighand = sighand_create()) == NULL) {
		goto bad_sighand;
	}

good_sighand:
	sighand_count_inc(sighand);
	proc->signal_info.sighand = sighand;
	return 0;

bad_sighand:
	return ret;
}

static void put_sighand(struct proc_struct *proc)
{
	struct sighand_struct *sh = proc->signal_info.sighand;
	if (sh != NULL) {
		if (sighand_count_dec(sh) == 0) {
			sighand_destroy(sh);
		}
	}
	proc->signal_info.sighand = NULL;
}

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

// may_killed - check if current thread should be killed, should be called before go back to user space
void may_killed(void)
{
	// killed by other process, already set exit_code and call __do_exit directly
	if (current->flags & PF_EXITING) {
		__do_exit();
	}
}

// do_fork - parent process for a new child process
//    1. call alloc_proc to allocate a proc_struct
//    2. call setup_kstack to allocate a kernel stack for child process
//    3. call copy_mm to dup OR share mm according clone_flag
//    4. call wakup_proc to make the new child process RUNNABLE 
int do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf)
{
	int ret = -E_NO_FREE_PROC;
	struct proc_struct *proc;
	if (nr_process >= MAX_PROCESS) {
		goto fork_out;
	}

	ret = -E_NO_MEM;

	if ((proc = alloc_proc()) == NULL) {
		goto fork_out;
	}
	if(clone_flags & __CLONE_PINCPU)
		proc->flags |= PF_PINCPU;

	proc->parent = current;
	list_init(&(proc->thread_group));
	assert(current->wait_state == 0);

	assert(current->time_slice >= 0);
	proc->time_slice = current->time_slice / 2;
	current->time_slice -= proc->time_slice;

	if (setup_kstack(proc) != 0) {
		goto bad_fork_cleanup_proc;
	}
	if (copy_sem(clone_flags, proc) != 0) {
		goto bad_fork_cleanup_kstack;
	}
	if (copy_fs(clone_flags, proc) != 0) {
		goto bad_fork_cleanup_sem;
	}
	if (copy_signal(clone_flags, proc) != 0) {
		goto bad_fork_cleanup_fs;
	}
	if (copy_sighand(clone_flags, proc) != 0) {
		goto bad_fork_cleanup_signal;
	}
	if (copy_mm(clone_flags, proc) != 0) {
		goto bad_fork_cleanup_sighand;
	}
	if (copy_thread(clone_flags, proc, stack, tf) != 0) {
		goto bad_fork_cleanup_sighand;
	}

	proc->tls_pointer = current->tls_pointer;

	bool intr_flag;
	spin_lock_irqsave(&proc_lock, intr_flag);
	{
		proc->pid = get_pid();
		proc->tid = proc->pid;
		hash_proc(proc);
		set_links(proc);
		if (clone_flags & CLONE_THREAD) {
			list_add_before(&(current->thread_group),
					&(proc->thread_group));
			proc->gid = current->gid;
		} else {
			proc->gid = proc->pid;
		}
	}
	spin_unlock_irqrestore(&proc_lock, intr_flag);

	wakeup_proc(proc);

	ret = proc->pid;
fork_out:
	return ret;
bad_fork_cleanup_sighand:
	put_sighand(proc);
bad_fork_cleanup_signal:
	put_signal(proc);
bad_fork_cleanup_fs:
	put_fs(proc);
bad_fork_cleanup_sem:
	put_sem_queue(proc);
bad_fork_cleanup_kstack:
	put_kstack(proc);
bad_fork_cleanup_proc:
	kfree(proc);
	goto fork_out;
}

// __do_exit - cause a thread exit (use do_exit, do_exit_thread instead)
//   1. call exit_mmap & put_pgdir & mm_destroy to free the almost all memory space of process
//   2. set process' state as PROC_ZOMBIE, then call wakeup_proc(parent) to ask parent reclaim itself.
//   3. call scheduler to switch to other process
static int __do_exit(void)
{
	if (current == idleproc) {
		panic("idleproc exit.\n");
	}
	if (current == initproc) {
		panic("initproc exit.\n");
	}

	struct mm_struct *mm = current->mm;
	if (mm != NULL) {
		mp_set_mm_pagetable(NULL);
		if (mm_count_dec(mm) == 0) {
			exit_mmap(mm);
			put_pgdir(mm);
			bool intr_flag;
			local_intr_save(intr_flag);
			{
				list_del(&(mm->proc_mm_link));
			}
			local_intr_restore(intr_flag);
			mm_destroy(mm);
		}
		current->mm = NULL;
	}
	put_sighand(current);
	put_signal(current);
	put_fs(current);
	put_sem_queue(current);
	current->state = PROC_ZOMBIE;

	bool intr_flag;
	struct proc_struct *proc, *parent;
	local_intr_save(intr_flag);
	{
		proc = parent = current->parent;
		do {
			if (proc->wait_state == WT_CHILD) {
				wakeup_proc(proc);
			}
			proc = next_thread(proc);
		} while (proc != parent);

		if ((parent = next_thread(current)) == current) {
			parent = initproc;
		}
		de_thread(current);
		while (current->cptr != NULL) {
			proc = current->cptr;
			current->cptr = proc->optr;

			proc->yptr = NULL;
			if ((proc->optr = parent->cptr) != NULL) {
				parent->cptr->yptr = proc;
			}
			proc->parent = parent;
			parent->cptr = proc;
			if (proc->state == PROC_ZOMBIE) {
				if (parent->wait_state == WT_CHILD) {
					wakeup_proc(parent);
				}
			}
		}
	}

	wakeup_queue(&(current->event_box.wait_queue), WT_INTERRUPTED, 1);

	local_intr_restore(intr_flag);

	schedule();
	panic("__do_exit will not return!! %d %d.\n", current->pid,
	      current->exit_code);
}

// do_exit - kill a thread group, called by syscall or trap handler
int do_exit(int error_code)
{
	bool intr_flag;
	local_intr_save(intr_flag);
	{
		list_entry_t *list = &(current->thread_group), *le = list;
		while ((le = list_next(le)) != list) {
			struct proc_struct *proc = le2proc(le, thread_group);
			__do_kill(proc, error_code);
		}
	}
	local_intr_restore(intr_flag);
	return do_exit_thread(error_code);
}

// do_exit_thread - kill a single thread
int do_exit_thread(int error_code)
{
	current->exit_code = error_code;
	return __do_exit();
}

static int load_icode_read(int fd, void *buf, size_t len, off_t offset)
{
	int ret;
	if ((ret = sysfile_seek(fd, offset, LSEEK_SET)) != 0) {
		return ret;
	}
	if ((ret = sysfile_read(fd, buf, len)) != len) {
		return (ret < 0) ? ret : -1;
	}
	return 0;
}

//#ifdef UCONFIG_BIONIC_LIBC
static int
map_ph(int fd, struct proghdr *ph, struct mm_struct *mm, uint32_t * pbias,
       uint32_t linker)
{
	int ret = 0;
	struct Page *page;
	uint32_t vm_flags = 0;
	uint32_t bias = 0;
	pte_perm_t perm = 0;
	ptep_set_u_read(&perm);

	if (ph->p_flags & ELF_PF_X)
		vm_flags |= VM_EXEC;
	if (ph->p_flags & ELF_PF_W)
		vm_flags |= VM_WRITE;
	if (ph->p_flags & ELF_PF_R)
		vm_flags |= VM_READ;

	if (vm_flags & VM_WRITE)
		ptep_set_u_write(&perm);

	if (pbias) {
		bias = *pbias;
	}
	if (!bias && !ph->p_va) {
		bias = get_unmapped_area(mm, ph->p_memsz + PGSIZE);
		bias = ROUNDUP(bias, PGSIZE);
		if (pbias)
			*pbias = bias;
	}

	if ((ret =
	     mm_map(mm, ph->p_va + bias, ph->p_memsz, vm_flags, NULL)) != 0) {
		goto bad_cleanup_mmap;
	}

	if (!linker && mm->brk_start < ph->p_va + bias + ph->p_memsz) {
		mm->brk_start = ph->p_va + bias + ph->p_memsz;
	}

	off_t offset = ph->p_offset;
	size_t off, size;
	uintptr_t start = ph->p_va + bias, end, la = ROUNDDOWN(start, PGSIZE);

	end = ph->p_va + bias + ph->p_filesz;
	while (start < end) {
		if ((page = pgdir_alloc_page(mm->pgdir, la, perm)) == NULL) {
			ret = -E_NO_MEM;
			goto bad_cleanup_mmap;
		}
		off = start - la, size = PGSIZE - off, la += PGSIZE;
		if (end < la) {
			size -= la - end;
		}
		if ((ret =
		     load_icode_read(fd, page2kva(page) + off, size,
				     offset)) != 0) {
			goto bad_cleanup_mmap;
		}
		start += size, offset += size;
	}

	end = ph->p_va + bias + ph->p_memsz;

	if (start < la) {
		if (start == end) {
			goto normal_exit;
		}
		off = start + PGSIZE - la, size = PGSIZE - off;
		if (end < la) {
			size -= la - end;
		}
		memset(page2kva(page) + off, 0, size);
		start += size;
		assert((end < la && start == end)
		       || (end >= la && start == la));
	}

	while (start < end) {
		if ((page = pgdir_alloc_page(mm->pgdir, la, perm)) == NULL) {
			ret = -E_NO_MEM;
			goto bad_cleanup_mmap;
		}
		off = start - la, size = PGSIZE - off, la += PGSIZE;
		if (end < la) {
			size -= la - end;
		}
		memset(page2kva(page) + off, 0, size);
		start += size;
	}
normal_exit:
	return 0;
bad_cleanup_mmap:
	return ret;
}

//#endif //UCONFIG_BIONIC_LIBC

static int load_icode(int fd, int argc, char **kargv, int envc, char **kenvp)
{
	assert(argc >= 0 && argc <= EXEC_MAX_ARG_NUM);
	assert(envc >= 0 && envc <= EXEC_MAX_ENV_NUM);
	if (current->mm != NULL) {
		panic("load_icode: current->mm must be empty.\n");
	}

	int ret = -E_NO_MEM;

//#ifdef UCONFIG_BIONIC_LIBC
	uint32_t real_entry;
//#endif //UCONFIG_BIONIC_LIBC

	struct mm_struct *mm;
	if ((mm = mm_create()) == NULL) {
		goto bad_mm;
	}

	if (setup_pgdir(mm) != 0) {
		goto bad_pgdir_cleanup_mm;
	}

	mm->brk_start = 0;

	struct Page *page;

	struct elfhdr __elf, *elf = &__elf;
	if ((ret = load_icode_read(fd, elf, sizeof(struct elfhdr), 0)) != 0) {
		goto bad_elf_cleanup_pgdir;
	}

	if (elf->e_magic != ELF_MAGIC) {
		ret = -E_INVAL_ELF;
		goto bad_elf_cleanup_pgdir;
	}
//#ifdef UCONFIG_BIONIC_LIBC
	real_entry = elf->e_entry;

	uint32_t load_address, load_address_flag = 0;
//#endif //UCONFIG_BIONIC_LIBC

	struct proghdr __ph, *ph = &__ph;
	uint32_t vm_flags, phnum;
	pte_perm_t perm = 0;

//#ifdef UCONFIG_BIONIC_LIBC
	uint32_t is_dynamic = 0, interp_idx;
	uint32_t bias = 0;
//#endif //UCONFIG_BIONIC_LIBC
	for (phnum = 0; phnum < elf->e_phnum; phnum++) {
		off_t phoff = elf->e_phoff + sizeof(struct proghdr) * phnum;
		if ((ret =
		     load_icode_read(fd, ph, sizeof(struct proghdr),
				     phoff)) != 0) {
			goto bad_cleanup_mmap;
		}

		if (ph->p_type == ELF_PT_INTERP) {
			is_dynamic = 1;
			interp_idx = phnum;
			continue;
		}

		if (ph->p_type != ELF_PT_LOAD) {
			continue;
		}
		if (ph->p_filesz > ph->p_memsz) {
			ret = -E_INVAL_ELF;
			goto bad_cleanup_mmap;
		}

		if (ph->p_va == 0 && !bias) {
			bias = 0x00008000;
		}

		if ((ret = map_ph(fd, ph, mm, &bias, 0)) != 0) {
			kprintf("load address: 0x%08x size: %d\n", ph->p_va,
				ph->p_memsz);
			goto bad_cleanup_mmap;
		}

		if (load_address_flag == 0)
			load_address = ph->p_va + bias;
		++load_address_flag;

	  /*********************************************/
		/*
		   vm_flags = 0;
		   ptep_set_u_read(&perm);
		   if (ph->p_flags & ELF_PF_X) vm_flags |= VM_EXEC;
		   if (ph->p_flags & ELF_PF_W) vm_flags |= VM_WRITE;
		   if (ph->p_flags & ELF_PF_R) vm_flags |= VM_READ;
		   if (vm_flags & VM_WRITE) ptep_set_u_write(&perm);

		   if ((ret = mm_map(mm, ph->p_va, ph->p_memsz, vm_flags, NULL)) != 0) {
		   goto bad_cleanup_mmap;
		   }

		   if (mm->brk_start < ph->p_va + ph->p_memsz) {
		   mm->brk_start = ph->p_va + ph->p_memsz;
		   }

		   off_t offset = ph->p_offset;
		   size_t off, size;
		   uintptr_t start = ph->p_va, end, la = ROUNDDOWN(start, PGSIZE);

		   end = ph->p_va + ph->p_filesz;
		   while (start < end) {
		   if ((page = pgdir_alloc_page(mm->pgdir, la, perm)) == NULL) {
		   ret = -E_NO_MEM;
		   goto bad_cleanup_mmap;
		   }
		   off = start - la, size = PGSIZE - off, la += PGSIZE;
		   if (end < la) {
		   size -= la - end;
		   }
		   if ((ret = load_icode_read(fd, page2kva(page) + off, size, offset)) != 0) {
		   goto bad_cleanup_mmap;
		   }
		   start += size, offset += size;
		   }

		   end = ph->p_va + ph->p_memsz;

		   if (start < la) {
		   // ph->p_memsz == ph->p_filesz 
		   if (start == end) {
		   continue ;
		   }
		   off = start + PGSIZE - la, size = PGSIZE - off;
		   if (end < la) {
		   size -= la - end;
		   }
		   memset(page2kva(page) + off, 0, size);
		   start += size;
		   assert((end < la && start == end) || (end >= la && start == la));
		   }

		   while (start < end) {
		   if ((page = pgdir_alloc_page(mm->pgdir, la, perm)) == NULL) {
		   ret = -E_NO_MEM;
		   goto bad_cleanup_mmap;
		   }
		   off = start - la, size = PGSIZE - off, la += PGSIZE;
		   if (end < la) {
		   size -= la - end;
		   }
		   memset(page2kva(page) + off, 0, size);
		   start += size;
		   }
		 */
	  /**************************************/
	}

	mm->brk_start = mm->brk = ROUNDUP(mm->brk_start, PGSIZE);

	/* setup user stack */
	vm_flags = VM_READ | VM_WRITE | VM_STACK;
	if ((ret =
	     mm_map(mm, USTACKTOP - USTACKSIZE, USTACKSIZE, vm_flags,
		    NULL)) != 0) {
		goto bad_cleanup_mmap;
	}

	if (is_dynamic) {
		elf->e_entry += bias;

		bias = 0;

		off_t phoff =
		    elf->e_phoff + sizeof(struct proghdr) * interp_idx;
		if ((ret =
		     load_icode_read(fd, ph, sizeof(struct proghdr),
				     phoff)) != 0) {
			goto bad_cleanup_mmap;
		}

		char *interp_path = (char *)kmalloc(ph->p_filesz);
		load_icode_read(fd, interp_path, ph->p_filesz, ph->p_offset);

		int interp_fd = sysfile_open(interp_path, O_RDONLY);
		assert(interp_fd >= 0);
		struct elfhdr interp___elf, *interp_elf = &interp___elf;
		assert((ret =
			load_icode_read(interp_fd, interp_elf,
					sizeof(struct elfhdr), 0)) == 0);
		assert(interp_elf->e_magic == ELF_MAGIC);

		struct proghdr interp___ph, *interp_ph = &interp___ph;
		uint32_t interp_phnum;
		uint32_t va_min = 0xffffffff, va_max = 0;
		for (interp_phnum = 0; interp_phnum < interp_elf->e_phnum;
		     ++interp_phnum) {
			off_t interp_phoff =
			    interp_elf->e_phoff +
			    sizeof(struct proghdr) * interp_phnum;
			assert((ret =
				load_icode_read(interp_fd, interp_ph,
						sizeof(struct proghdr),
						interp_phoff)) == 0);
			if (interp_ph->p_type != ELF_PT_LOAD) {
				continue;
			}
			if (va_min > interp_ph->p_va)
				va_min = interp_ph->p_va;
			if (va_max < interp_ph->p_va + interp_ph->p_memsz)
				va_max = interp_ph->p_va + interp_ph->p_memsz;
		}

		bias = get_unmapped_area(mm, va_max - va_min + 1 + PGSIZE);
		bias = ROUNDUP(bias, PGSIZE);

		for (interp_phnum = 0; interp_phnum < interp_elf->e_phnum;
		     ++interp_phnum) {
			off_t interp_phoff =
			    interp_elf->e_phoff +
			    sizeof(struct proghdr) * interp_phnum;
			assert((ret =
				load_icode_read(interp_fd, interp_ph,
						sizeof(struct proghdr),
						interp_phoff)) == 0);
			if (interp_ph->p_type != ELF_PT_LOAD) {
				continue;
			}
			assert((ret =
				map_ph(interp_fd, interp_ph, mm, &bias,
				       1)) == 0);
		}

		real_entry = interp_elf->e_entry + bias;

		sysfile_close(interp_fd);
		kfree(interp_path);
	}

	sysfile_close(fd);

	bool intr_flag;
	local_intr_save(intr_flag);
	{
		list_add(&(proc_mm_list), &(mm->proc_mm_link));
	}
	local_intr_restore(intr_flag);
	mm_count_inc(mm);
	current->mm = mm;
	set_pgdir(current, mm->pgdir);
	mp_set_mm_pagetable(mm);

	if (!is_dynamic) {
		real_entry += bias;
	}
#ifdef UCONFIG_BIONIC_LIBC
	if (init_new_context_dynamic(current, elf, argc, kargv, envc, kenvp,
				     is_dynamic, real_entry, load_address,
				     bias) < 0)
		goto bad_cleanup_mmap;
#else
	if (init_new_context(current, elf, argc, kargv, envc, kenvp) < 0)
		goto bad_cleanup_mmap;
#endif //UCONFIG_BIONIC_LIBC
	ret = 0;
out:
	return ret;
bad_cleanup_mmap:
	exit_mmap(mm);
bad_elf_cleanup_pgdir:
	put_pgdir(mm);
bad_pgdir_cleanup_mm:
	mm_destroy(mm);
bad_mm:
	goto out;
}

static void put_kargv(int argc, char **kargv)
{
	while (argc > 0) {
		kfree(kargv[--argc]);
	}
}

static int
copy_kargv(struct mm_struct *mm, char **kargv, const char **argv, int max_argc,
	   int *argc_store)
{
	int i, ret = -E_INVAL;
	if (!argv) {
		*argc_store = 0;
		return 0;
	}
	char *argv_k;
	for (i = 0; i < max_argc; i++) {
		if (!copy_from_user(mm, &argv_k, argv + i, sizeof(char *), 0))
			goto failed_cleanup;
		if (!argv_k)
			break;
		char *buffer;
		if ((buffer = kmalloc(EXEC_MAX_ARG_LEN + 1)) == NULL) {
			goto failed_nomem;
		}
#if 0
		if (!copy_from_user(mm, &argv_k, argv + i, sizeof(char *), 0) ||
		    !copy_string(mm, buffer, argv_k, EXEC_MAX_ARG_LEN + 1)) {
			kfree(buffer);
			goto failed_cleanup;
		}
#endif
		if (!copy_string(mm, buffer, argv_k, EXEC_MAX_ARG_LEN + 1)) {
			kfree(buffer);
			goto failed_cleanup;
		}
		kargv[i] = buffer;
	}
	*argc_store = i;
	return 0;

failed_nomem:
	ret = -E_NO_MEM;
failed_cleanup:
	put_kargv(i, kargv);
	return ret;
}

int do_execve(const char *filename, const char **argv, const char **envp)
{
	static_assert(EXEC_MAX_ARG_LEN >= FS_MAX_FPATH_LEN);

	struct mm_struct *mm = current->mm;

	char local_name[PROC_NAME_LEN + 1];
	memset(local_name, 0, sizeof(local_name));

	char *kargv[EXEC_MAX_ARG_NUM], *kenvp[EXEC_MAX_ENV_NUM];
	const char *path;

	int ret = -E_INVAL;
	lock_mm(mm);
#if 0
	if (name == NULL) {
		snprintf(local_name, sizeof(local_name), "<null> %d",
			 current->pid);
	} else {
		if (!copy_string(mm, local_name, name, sizeof(local_name))) {
			unlock_mm(mm);
			return ret;
		}
	}
#endif
	snprintf(local_name, sizeof(local_name), "<null> %d", current->pid);

	int argc = 0, envc = 0;
	if ((ret = copy_kargv(mm, kargv, argv, EXEC_MAX_ARG_NUM, &argc)) != 0) {
		unlock_mm(mm);
		return ret;
	}
	if ((ret = copy_kargv(mm, kenvp, envp, EXEC_MAX_ENV_NUM, &envc)) != 0) {
		unlock_mm(mm);
		put_kargv(argc, kargv);
		return ret;
	}
#if 0
	int i;
	kprintf("## fn %s\n", filename);
	kprintf("## argc %d\n", argc);
	for (i = 0; i < argc; i++)
		kprintf("## %08x %s\n", kargv[i], kargv[i]);
	kprintf("## envc %d\n", envc);
	for (i = 0; i < envc; i++)
		kprintf("## %08x %s\n", kenvp[i], kenvp[i]);
#endif
	//path = argv[0];
	//copy_from_user (mm, &path, argv, sizeof (char*), 0);
	path = filename;
	unlock_mm(mm);

	/* linux never do this */
	//fs_closeall(current->fs_struct);

	/* sysfile_open will check the first argument path, thus we have to use a user-space pointer, and argv[0] may be incorrect */

	int fd;
	if ((ret = fd = sysfile_open(path, O_RDONLY)) < 0) {
		goto execve_exit;
	}

	if (mm != NULL) {
		mp_set_mm_pagetable(NULL);
		if (mm_count_dec(mm) == 0) {
			exit_mmap(mm);
			put_pgdir(mm);
			bool intr_flag;
			local_intr_save(intr_flag);
			{
				list_del(&(mm->proc_mm_link));
			}
			local_intr_restore(intr_flag);
			mm_destroy(mm);
		}
		current->mm = NULL;
	}
	put_sem_queue(current);

	ret = -E_NO_MEM;
	/* init signal */
	put_sighand(current);
	if ((current->signal_info.sighand = sighand_create()) == NULL) {
		goto execve_exit;
	}
	sighand_count_inc(current->signal_info.sighand);

	put_signal(current);
	if ((current->signal_info.signal = signal_create()) == NULL) {
		goto execve_exit;
	}
	signal_count_inc(current->signal_info.signal);

	if ((current->sem_queue = sem_queue_create()) == NULL) {
		goto execve_exit;
	}
	sem_queue_count_inc(current->sem_queue);

	if ((ret = load_icode(fd, argc, kargv, envc, kenvp)) != 0) {
		goto execve_exit;
	}

	set_proc_name(current, local_name);

	if (do_execve_arch_hook(argc, kargv) < 0)
		goto execve_exit;

	put_kargv(argc, kargv);
	put_kargv(envc, kenvp);
	return 0;

execve_exit:
	put_kargv(argc, kargv);
	put_kargv(envc, kenvp);
/* exec should return -1 if failed */
	//return ret;
	do_exit(ret);
	panic("already exit: %e.\n", ret);
}

// do_yield - ask the scheduler to reschedule
int do_yield(void)
{
	current->need_resched = 1;
	return 0;
}

// do_wait - wait one OR any children with PROC_ZOMBIE state, and free memory space of kernel stack
//         - proc struct of this child.
// NOTE: only after do_wait function, all resources of the child proces are free.
int do_wait(int pid, int *code_store)
{
	struct mm_struct *mm = current->mm;
	if (code_store != NULL) {
		if (!user_mem_check(mm, (uintptr_t) code_store, sizeof(int), 1)) {
			return -E_INVAL;
		}
	}

	struct proc_struct *proc, *cproc;
	bool intr_flag, haskid;
repeat:
	cproc = current;
	haskid = 0;
	if (pid != 0) {
		proc = find_proc(pid);
		if (proc != NULL) {
			do {
				if (proc->parent == cproc) {
					haskid = 1;
					if (proc->state == PROC_ZOMBIE) {
						goto found;
					}
					break;
				}
				cproc = next_thread(cproc);
			} while (cproc != current);
		}
	} else {
		do {
			proc = cproc->cptr;
			for (; proc != NULL; proc = proc->optr) {
				haskid = 1;
				if (proc->state == PROC_ZOMBIE) {
					goto found;
				}
			}
			cproc = next_thread(cproc);
		} while (cproc != current);
	}
	if (haskid) {
		current->state = PROC_SLEEPING;
		current->wait_state = WT_CHILD;
		schedule();
		may_killed();
		goto repeat;
	}
	return -E_BAD_PROC;

found:
	if (proc == idleproc || proc == initproc) {
		panic("wait idleproc or initproc.\n");
	}
	int exit_code = proc->exit_code;
	spin_lock_irqsave(&proc_lock, intr_flag);
	{
		unhash_proc(proc);
		remove_links(proc);
	}
	spin_unlock_irqrestore(&proc_lock, intr_flag);
	put_kstack(proc);
	kfree(proc);

	int ret = 0;
	if (code_store != NULL) {
		lock_mm(mm);
		{
			if (!copy_to_user
			    (mm, code_store, &exit_code, sizeof(int))) {
				ret = -E_INVAL;
			}
		}
		unlock_mm(mm);
	}
	return ret;
}

int do_linux_waitpid(int pid, int *code_store)
{
	struct mm_struct *mm = current->mm;
	if (code_store != NULL) {
		if (!user_mem_check(mm, (uintptr_t) code_store, sizeof(int), 1)) {
			return -E_INVAL;
		}
	}

	struct proc_struct *proc, *cproc;
	bool intr_flag, haskid;
repeat:
	cproc = current;
	haskid = 0;
	if (pid > 0) {
		proc = find_proc(pid);
		if (proc != NULL) {
			do {
				if (proc->parent == cproc) {
					haskid = 1;
					if (proc->state == PROC_ZOMBIE) {
						goto found;
					}
					break;
				}
				cproc = next_thread(cproc);
			} while (cproc != current);
		}
	}
	/* we do NOT have group id, so.. */
	else if (pid == 0 || pid == -1) {	/* pid == 0 */
		do {
			proc = cproc->cptr;
			for (; proc != NULL; proc = proc->optr) {
				haskid = 1;
				if (proc->state == PROC_ZOMBIE) {
					goto found;
				}
			}
			cproc = next_thread(cproc);
		} while (cproc != current);
	} else {		//pid<-1
		//TODO
		return -E_INVAL;
	}
	if (haskid) {
		current->state = PROC_SLEEPING;
		current->wait_state = WT_CHILD;
		schedule();
		may_killed();
		goto repeat;
	}
	return -E_BAD_PROC;

found:
	if (proc == idleproc || proc == initproc) {
		panic("wait idleproc or initproc.\n");
	}
	int exit_code = proc->exit_code;
	int return_pid = proc->pid;
	local_intr_save(intr_flag);
	{
		unhash_proc(proc);
		remove_links(proc);
	}
	local_intr_restore(intr_flag);
	put_kstack(proc);
	kfree(proc);

	int ret = 0;
	if (code_store != NULL) {
		lock_mm(mm);
		{
			int status = exit_code << 8;
			if (!copy_to_user(mm, code_store, &status, sizeof(int))) {
				ret = -E_INVAL;
			}
		}
		unlock_mm(mm);
	}
	return (ret == 0) ? return_pid : ret;
}

// __do_kill - kill a process with PCB by set this process's flags with PF_EXITING
static int __do_kill(struct proc_struct *proc, int error_code)
{
	if (!(proc->flags & PF_EXITING)) {
		proc->flags |= PF_EXITING;
		proc->exit_code = error_code;
		if (proc->wait_state & WT_INTERRUPTED) {
			wakeup_proc(proc);
		}
		return 0;
	}
	return -E_KILLED;
}

// do_kill - kill process with pid
int do_kill(int pid, int error_code)
{
	struct proc_struct *proc;
	if ((proc = find_proc(pid)) != NULL) {
		return __do_kill(proc, error_code);
	}
	return -E_INVAL;
}

// do_brk - adjust(increase/decrease) the size of process heap, align with page size
// NOTE: will change the process vma
int do_brk(uintptr_t * brk_store)
{
	struct mm_struct *mm = current->mm;
	if (mm == NULL) {
		panic("kernel thread call sys_brk!!.\n");
	}
	if (brk_store == NULL) {
		//     return -E_INVAL;
		return mm->brk_start;
	}

	uintptr_t brk;

	lock_mm(mm);
	if (!copy_from_user(mm, &brk, brk_store, sizeof(uintptr_t), 1)) {
		unlock_mm(mm);
		return -E_INVAL;
	}

	if (brk < mm->brk_start) {
		goto out_unlock;
	}
	uintptr_t newbrk = ROUNDUP(brk, PGSIZE), oldbrk = mm->brk;
	assert(oldbrk % PGSIZE == 0);
	if (newbrk == oldbrk) {
		goto out_unlock;
	}
	if (newbrk < oldbrk) {
		if (mm_unmap(mm, newbrk, oldbrk - newbrk) != 0) {
			goto out_unlock;
		}
	} else {
		if (find_vma_intersection(mm, oldbrk, newbrk + PGSIZE) != NULL) {
			goto out_unlock;
		}
		if (mm_brk(mm, oldbrk, newbrk - oldbrk) != 0) {
			goto out_unlock;
		}
	}
	mm->brk = newbrk;
out_unlock:
	copy_to_user(mm, brk_store, &mm->brk, sizeof(uintptr_t));
	unlock_mm(mm);
	return 0;
}

/* poring from linux */
int do_linux_brk(uintptr_t brk)
{
	uint32_t newbrk, oldbrk, retval;
	struct mm_struct *mm = current->mm;
	uint32_t min_brk;

	if (!mm) {
		panic("kernel thread call sys_brk!!.\n");
	}

	lock_mm(mm);

	min_brk = mm->brk_start;

	if (brk < min_brk)
		goto out_unlock;

	newbrk = ROUNDUP(brk, PGSIZE);
	oldbrk = ROUNDUP(mm->brk, PGSIZE);

	if (oldbrk == newbrk)
		goto set_brk;

	if (brk <= mm->brk) {
		if (!mm_unmap(mm, newbrk, oldbrk - newbrk))
			goto set_brk;
		goto out_unlock;
	}

	if (find_vma_intersection(mm, oldbrk, newbrk + PGSIZE))
		goto out_unlock;

	/* set the brk */
	if (mm_brk(mm, oldbrk, newbrk - oldbrk))
		goto out_unlock;

set_brk:
	mm->brk = brk;
out_unlock:
	retval = mm->brk;
	unlock_mm(mm);
	return retval;
}

/* from x86 bionic porting */
/*
int
do_linux_brk(uintptr_t brk) {
    struct mm_struct *mm = current->mm;
    if (mm == NULL) {
        panic("kernel thread call sys_brk!!.\n");
    }

    if (brk == 0) {
        return mm->brk_start;
    }

    lock_mm(mm);
    if (brk < mm->brk_start) {
        goto out_unlock;
    }
    uintptr_t newbrk = ROUNDUP(brk, PGSIZE), oldbrk = mm->brk;
    assert(oldbrk % PGSIZE == 0);
    if (newbrk == oldbrk) {
        goto out_unlock;
    }
    if (newbrk < oldbrk) {
        if (mm_unmap(mm, newbrk, oldbrk - newbrk) != 0) {
            goto out_unlock;
        }
    }
    else {
        if (find_vma_intersection(mm, oldbrk, newbrk + PGSIZE) != NULL) {
            goto out_unlock;
        }
        if (mm_brk(mm, oldbrk, newbrk - oldbrk) != 0) {
            goto out_unlock;
        }
    }
    mm->brk = newbrk;
out_unlock:
    unlock_mm(mm);
    return newbrk;
}
*/

// do_sleep - set current process state to sleep and add timer with "time"
//          - then call scheduler. if process run again, delete timer first.
//      time is jiffies
int do_sleep(unsigned int time)
{
	assert(!ucore_in_interrupt());
	if (time == 0) {
		return 0;
	}
	bool intr_flag;
	local_intr_save(intr_flag);
	timer_t __timer, *timer = timer_init(&__timer, current, time);
	current->state = PROC_SLEEPING;
	current->wait_state = WT_TIMER;
	add_timer(timer);
	local_intr_restore(intr_flag);

	schedule();

	del_timer(timer);
	return 0;
}

int do_linux_sleep(const struct linux_timespec __user * req,
		   struct linux_timespec __user * rem)
{
	struct mm_struct *mm = current->mm;
	struct linux_timespec kts;
	lock_mm(mm);
	if (!copy_from_user(mm, &kts, req, sizeof(struct linux_timespec), 1)) {
		unlock_mm(mm);
		return -E_INVAL;
	}
	unlock_mm(mm);
	long msec = kts.tv_sec * 1000 + kts.tv_nsec / 1000000;
	if (msec < 0)
		return -E_INVAL;
#ifdef UCONFIG_HAVE_LINUX_DDE_BASE
	unsigned long j = msecs_to_jiffies(msec);
#else
	unsigned long j = msec / 10;
#endif
	//kprintf("do_linux_sleep: sleep %d msec, %d jiffies\n", msec, j);
	int ret = do_sleep(j);
	if (rem) {
		memset(&kts, 0, sizeof(struct linux_timespec));
		lock_mm(mm);
		if (!copy_to_user(mm, rem, &kts, sizeof(struct linux_timespec))) {
			unlock_mm(mm);
			return -E_INVAL;
		}
		unlock_mm(mm);
	}
	return ret;
}

int
__do_linux_mmap(uintptr_t __user * addr_store, size_t len, uint32_t mmap_flags)
{
	struct mm_struct *mm = current->mm;
	if (mm == NULL) {
		panic("kernel thread call mmap!!.\n");
	}
	if (addr_store == NULL || len == 0) {
		return -E_INVAL;
	}

	int ret = -E_INVAL;

	uintptr_t addr;
	addr = *addr_store;

	uintptr_t start = ROUNDDOWN(addr, PGSIZE), end =
	    ROUNDUP(addr + len, PGSIZE);
	addr = start, len = end - start;

	uint32_t vm_flags = VM_READ;

	/* set anonymous flag */
	vm_flags |= VM_ANONYMOUS;

	if (mmap_flags & MMAP_WRITE)
		vm_flags |= VM_WRITE;
	if (mmap_flags & MMAP_STACK)
		vm_flags |= VM_STACK;

	ret = -E_NO_MEM;
	if (addr == 0) {
		if ((addr = get_unmapped_area(mm, len)) == 0) {
			goto out_unlock;
		}
	}
	if ((ret = mm_map(mm, addr, len, vm_flags, NULL)) == 0) {
		*addr_store = addr;
	}
out_unlock:
	return ret;
}

// do_mmap - add a vma with addr, len and flags(VM_READ/M_WRITE/VM_STACK)
int do_mmap(uintptr_t __user * addr_store, size_t len, uint32_t mmap_flags)
{
	struct mm_struct *mm = current->mm;
	if (mm == NULL) {
		panic("kernel thread call mmap!!.\n");
	}
	if (addr_store == NULL || len == 0) {
		return -E_INVAL;
	}

	int ret = -E_INVAL;

	uintptr_t addr;

	lock_mm(mm);
	if (!copy_from_user(mm, &addr, addr_store, sizeof(uintptr_t), 1)) {
		goto out_unlock;
	}

	uintptr_t start = ROUNDDOWN(addr, PGSIZE), end =
	    ROUNDUP(addr + len, PGSIZE);
	addr = start, len = end - start;

	uint32_t vm_flags = VM_READ;
	if (mmap_flags & MMAP_WRITE)
		vm_flags |= VM_WRITE;
	if (mmap_flags & MMAP_STACK)
		vm_flags |= VM_STACK;

	ret = -E_NO_MEM;
	if (addr == 0) {
		if ((addr = get_unmapped_area(mm, len)) == 0) {
			goto out_unlock;
		}
	}
	if ((ret = mm_map(mm, addr, len, vm_flags, NULL)) == 0) {
		copy_to_user(mm, addr_store, &addr, sizeof(uintptr_t));
	}
out_unlock:
	unlock_mm(mm);
	return ret;
}

// do_munmap - delete vma with addr & len
int do_munmap(uintptr_t addr, size_t len)
{
	struct mm_struct *mm = current->mm;
	if (mm == NULL) {
		panic("kernel thread call munmap!!.\n");
	}
	if (len == 0) {
		return -E_INVAL;
	}
	int ret;
	lock_mm(mm);
	{
		ret = mm_unmap(mm, addr, len);
	}
	unlock_mm(mm);
	return ret;
}

#ifdef UCONFIG_BIONIC_LIBC

int do_mprotect(void *addr, size_t len, int prot)
{

	/*
	   return 0; 
	 */

	struct mm_struct *mm = current->mm;
	assert(mm != NULL);
	if (len == 0) {
		return -E_INVAL;
	}
	uintptr_t start = ROUNDDOWN(addr, PGSIZE);
	uintptr_t end = ROUNDUP(addr + len, PGSIZE);

	int ret = -E_INVAL;
	lock_mm(mm);

	while (1) {
		struct vma_struct *vma = find_vma(mm, start);
		uintptr_t last_end;
		if (vma != NULL) {
			last_end = vma->vm_end;
		}
		if (vma == NULL) {
			goto out;
		} else if (vma->vm_start == start && vma->vm_end == end) {
			if (prot & PROT_WRITE) {
				vma->vm_flags |= VM_WRITE;
			} else {
				vma->vm_flags &= ~VM_WRITE;
			}
		} else {
			uintptr_t this_end =
			    (end <= vma->vm_end) ? end : vma->vm_end;
			uintptr_t this_start =
			    (start >= vma->vm_start) ? start : vma->vm_start;

			struct mapped_file_struct mfile = vma->mfile;
			mfile.offset += this_start - vma->vm_start;
			uint32_t flags = vma->vm_flags;
			if ((ret =
			     mm_unmap_keep_pages(mm, this_start,
						 this_end - this_start)) != 0) {
				goto out;
			}
			if (prot & PROT_WRITE) {
				flags |= VM_WRITE;
			} else {
				flags &= ~VM_WRITE;
			}
			if ((ret =
			     mm_map(mm, this_start, this_end - this_start,
				    flags, &vma)) != 0) {
				goto out;
			}
			vma->mfile = mfile;
			if (vma->mfile.file != NULL) {
				filemap_acquire(mfile.file);
			}
		}

		ret = 0;

		if (end <= last_end)
			break;
		start = last_end;
	}

out:
	unlock_mm(mm);
	return ret;
}

#endif //UCONFIG_BIONIC_LIBC

// do_shmem - create a share memory with addr, len, flags(VM_READ/M_WRITE/VM_STACK)
int do_shmem(uintptr_t * addr_store, size_t len, uint32_t mmap_flags)
{
	struct mm_struct *mm = current->mm;
	if (mm == NULL) {
		panic("kernel thread call mmap!!.\n");
	}
	if (addr_store == NULL || len == 0) {
		return -E_INVAL;
	}

	int ret = -E_INVAL;

	uintptr_t addr;

	lock_mm(mm);
	if (!copy_from_user(mm, &addr, addr_store, sizeof(uintptr_t), 1)) {
		goto out_unlock;
	}

	uintptr_t start = ROUNDDOWN(addr, PGSIZE), end =
	    ROUNDUP(addr + len, PGSIZE);
	addr = start, len = end - start;

	uint32_t vm_flags = VM_READ;
	if (mmap_flags & MMAP_WRITE)
		vm_flags |= VM_WRITE;
	if (mmap_flags & MMAP_STACK)
		vm_flags |= VM_STACK;

	ret = -E_NO_MEM;
	if (addr == 0) {
		if ((addr = get_unmapped_area(mm, len)) == 0) {
			goto out_unlock;
		}
	}
	struct shmem_struct *shmem;
	if ((shmem = shmem_create(len)) == NULL) {
		goto out_unlock;
	}
	if ((ret = mm_map_shmem(mm, addr, vm_flags, shmem, NULL)) != 0) {
		assert(shmem_ref(shmem) == 0);
		shmem_destroy(shmem);
		goto out_unlock;
	}
	copy_to_user(mm, addr_store, &addr, sizeof(uintptr_t));
out_unlock:
	unlock_mm(mm);
	return ret;
}

#define __KERNEL_EXECVE(name, path, ...) ({                         \
            const char *argv[] = {path, ##__VA_ARGS__, NULL};       \
            const char *envp[] = {"PATH=/bin/", NULL};              \
            kprintf("kernel_execve: pid = %d, name = \"%s\".\n",    \
                    current->pid, name);                            \
            kernel_execve(path, argv, envp);                              \
        })

#define KERNEL_EXECVE(x, ...)                   __KERNEL_EXECVE(#x, #x, ##__VA_ARGS__)

#define KERNEL_EXECVE2(x, ...)                  KERNEL_EXECVE(x, ##__VA_ARGS__)

#define __KERNEL_EXECVE3(x, s, ...)             KERNEL_EXECVE(x, #s, ##__VA_ARGS__)

#define KERNEL_EXECVE3(x, s, ...)               __KERNEL_EXECVE3(x, s, ##__VA_ARGS__)

// user_main - kernel thread used to exec a user program
static int user_main(void *arg)
{
	sysfile_open("stdin:", O_RDONLY);
	sysfile_open("stdout:", O_WRONLY);
	sysfile_open("stdout:", O_WRONLY);
#ifdef UNITTEST
#ifdef TESTSCRIPT
	KERNEL_EXECVE3(UNITTEST, TESTSCRIPT);
#else
	KERNEL_EXECVE2(UNITTEST);
#endif
#else
	__KERNEL_EXECVE("/bin/sh", "/bin/sh");
#endif
	kprintf("user_main execve failed, no /bin/sh?.\n");
}

// init_main - the second kernel thread used to create kswapd_main & user_main kernel threads
static int init_main(void *arg)
{
	int pid;
#ifdef UCONFIG_SWAP
	if ((pid = ucore_kernel_thread(kswapd_main, NULL, 0)) <= 0) {
		panic("kswapd init failed.\n");
	}
	kswapd = find_proc(pid);
	set_proc_name(kswapd, "kswapd");
#else
	kprintf("init_main:: swapping is disabled.\n");
#endif

	int ret;
	char root[] = "disk0:";
	if ((ret = vfs_set_bootfs(root)) != 0) {
		panic("set boot fs failed: %e.\n", ret);
	}

	size_t nr_used_pages_store = nr_used_pages();
	size_t slab_allocated_store = slab_allocated();

	unsigned int nr_process_store = nr_process;

	pid = ucore_kernel_thread(user_main, NULL, 0);
	if (pid <= 0) {
		panic("create user_main failed.\n");
	}

	while (do_wait(0, NULL) == 0) {
		if (nr_process_store == nr_process) {
			break;
		}
		schedule();
	}
#ifdef UCONFIG_SWAP
	assert(kswapd != NULL);
	int i;
	for (i = 0; i < 10; i++) {
		if (kswapd->wait_state == WT_TIMER) {
			wakeup_proc(kswapd);
		}
		schedule();
	}
#endif

	mbox_cleanup();
	fs_cleanup();

	kprintf("all user-mode processes have quit.\n");
#ifdef UCONFIG_SWAP
	assert(initproc->cptr == kswapd && initproc->yptr == NULL
	       && initproc->optr == NULL);
	assert(kswapd->cptr == NULL && kswapd->yptr == NULL
	       && kswapd->optr == NULL);
	assert(nr_process == 2 + sysconf.lcpu_count);
#else
	assert(nr_process == 1 + sysconf.lcpu_count);
#endif
	assert(nr_used_pages_store == nr_used_pages());
	assert(slab_allocated_store == slab_allocated());
	kprintf("init check memory pass.\n");
	return 0;
}

// proc_init - set up the first kernel thread idleproc "idle" by itself and 
//           - create the second kernel thread init_main
void proc_init(void)
{
	int i;
	int cpuid = myid();
	struct proc_struct *idle;

	spinlock_init(&proc_lock);
	list_init(&proc_list);
	list_init(&proc_mm_list);
	for (i = 0; i < HASH_LIST_SIZE; i++) {
		list_init(hash_list + i);
	}

	idle = alloc_proc();
	if (idle == NULL) {
		panic("cannot alloc idleproc.\n");
	}

	idle->pid = cpuid;
	idle->state = PROC_RUNNABLE;
	// No need to be set for kthread (no privilege switch)
	// idleproc->kstack = (uintptr_t)bootstack;
	idle->need_resched = 1;
	idle->tf = NULL;
	if ((idle->fs_struct = fs_create()) == NULL) {
		panic("create fs_struct (idleproc) failed.\n");
	}
	fs_count_inc(idle->fs_struct);

	char namebuf[32];
	snprintf(namebuf, 32, "idle/%d", cpuid);

	set_proc_name(idle, namebuf);
	nr_process++;

	idleproc = idle;
	current = idle;

	int pid = ucore_kernel_thread(init_main, NULL, 0);
	if (pid <= 0) {
		panic("create init_main failed.\n");
	}

	initproc = find_proc(pid);
	set_proc_name(initproc, "kinit");

	assert(idleproc != NULL && idleproc->pid == cpuid);
	assert(initproc != NULL && initproc->pid == sysconf.lcpu_count);
}

void proc_init_ap(void)
{
	int cpuid = myid();
	struct proc_struct *idle;

	idle = alloc_proc();
	if (idle == NULL) {
		panic("cannot alloc idleproc.\n");
	}

	idle->pid = cpuid;
	idle->state = PROC_RUNNABLE;
	// No need to be set for kthread (no privilege switch)
	// idle->kstack = (uintptr_t)bootstack;
	idle->need_resched = 1;
	idle->tf = NULL;
	if ((idle->fs_struct = fs_create()) == NULL) {
		panic("create fs_struct (idleproc) failed.\n");
	}
	fs_count_inc(idle->fs_struct);

	char namebuf[32];
	snprintf(namebuf, 32, "idle/%d", cpuid);

	set_proc_name(idle, namebuf);
	nr_process++;

	idleproc = idle;
	current = idle;
#if 1
	int pid;
	char proc_name[32];
	if((pid = ucore_kernel_thread(krefcache_cleaner, NULL, 0)) <= 0){
		panic("krefcache_cleaner init failed.\n");
	}
	struct proc_struct* cleaner = find_proc(pid);
	snprintf(proc_name, 32, "krefcache/%d", myid());
	set_proc_name(cleaner, proc_name);
	set_proc_cpu_affinity(cleaner, myid());
	nr_process++;
#endif

	assert(idleproc != NULL && idleproc->pid == cpuid);
}

int do_linux_ugetrlimit(int res, struct linux_rlimit *__user __limit)
{
	struct linux_rlimit limit;
	switch (res) {
	case RLIMIT_STACK:
		limit.rlim_cur = USTACKSIZE;
		limit.rlim_max = USTACKSIZE;
		break;
	default:
		return -E_INVAL;
	}
	struct mm_struct *mm = current->mm;
	lock_mm(mm);
	int ret = 0;
	if (!copy_to_user(mm, __limit, &limit, sizeof(struct linux_rlimit))) {
		ret = -E_INVAL;
	}
	unlock_mm(mm);
	return ret;
}
