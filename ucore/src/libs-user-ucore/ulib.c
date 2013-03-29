#include <types.h>
#include <string.h>
#include <syscall.h>
#include <stdio.h>
#include <ulib.h>
#include <stat.h>
#include <lock.h>

static lock_t fork_lock = INIT_LOCK;

void lock_fork(void)
{
	lock(&fork_lock);
}

void unlock_fork(void)
{
	unlock(&fork_lock);
}

void exit(int error_code)
{
	sys_exit(error_code);
	cprintf("BUG: exit failed.\n");
	while (1) ;
}

int fork(void)
{
	int ret;
	lock_fork();
	ret = sys_fork();
	unlock_fork();
	return ret;
}

int wait(void)
{
	return sys_wait(0, NULL);
}

int waitpid(int pid, int *store)
{
	return sys_wait(pid, store);
}

void yield(void)
{
	sys_yield();
}

int sleep(unsigned int time)
{
	return sys_sleep(time);
}

int kill(int pid)
{
	return sys_kill(pid);
}

unsigned int gettime_msec(void)
{
	return (unsigned int)sys_gettime();
}

int getpid(void)
{
	return sys_getpid();
}

//print_pgdir - print the PDT&PT
void print_pgdir(void)
{
	sys_pgdir();
}

int mmap(uintptr_t * addr_store, size_t len, uint32_t mmap_flags)
{
	return sys_mmap(addr_store, len, mmap_flags);
}

int munmap(uintptr_t addr, size_t len)
{
	return sys_munmap(addr, len);
}

int shmem(uintptr_t * addr_store, size_t len, uint32_t mmap_flags)
{
	return sys_shmem(addr_store, len, mmap_flags);
}

sem_t sem_init(int value)
{
	return sys_sem_init(value);
}

int sem_post(sem_t sem_id)
{
	return sys_sem_post(sem_id);
}

int sem_wait(sem_t sem_id)
{
	return sys_sem_wait(sem_id, 0);
}

int sem_wait_timeout(sem_t sem_id, unsigned int timeout)
{
	return sys_sem_wait(sem_id, timeout);
}

int sem_free(sem_t sem_id)
{
	return sys_sem_free(sem_id);
}

int sem_get_value(sem_t sem_id, int *value_store)
{
	return sys_sem_get_value(sem_id, value_store);
}

int send_event(int pid, int event)
{
	return sys_send_event(pid, event, 0);
}

int send_event_timeout(int pid, int event, unsigned int timeout)
{
	return sys_send_event(pid, event, timeout);
}

int recv_event(int *pid_store, int *event_store)
{
	return sys_recv_event(pid_store, event_store, 0);
}

int recv_event_timeout(int *pid_store, int *event_store, unsigned int timeout)
{
	return sys_recv_event(pid_store, event_store, timeout);
}

int mbox_init(unsigned int max_slots)
{
	return sys_mbox_init(max_slots);
}

int mbox_send(int id, struct mboxbuf *buf)
{
	return sys_mbox_send(id, buf, 0);
}

int mbox_send_timeout(int id, struct mboxbuf *buf, unsigned int timeout)
{
	return sys_mbox_send(id, buf, timeout);
}

int mbox_recv(int id, struct mboxbuf *buf)
{
	return sys_mbox_recv(id, buf, 0);
}

int mbox_recv_timeout(int id, struct mboxbuf *buf, unsigned int timeout)
{
	return sys_mbox_recv(id, buf, timeout);
}

int mbox_free(int id)
{
	return sys_mbox_free(id);
}

int mbox_info(int id, struct mboxinfo *info)
{
	return sys_mbox_info(id, info);
}

int __exec(const char *name, const char **argv, const char **envp)
{
	int argc = 0;
	while (argv[argc] != NULL) {
		argc++;
	}
	return sys_exec(argv[0], argv, envp);
}

int __clone(uint32_t clone_flags, uintptr_t stack, int (*fn) (void *),
	    void *arg);

int clone(uint32_t clone_flags, uintptr_t stack, int (*fn) (void *), void *arg)
{
	int ret;
	lock_fork();
	ret = __clone(clone_flags, stack, fn, arg);
	unlock_fork();
	return ret;
}

void raise()
{
	exit(-1);
}

void halt()
{
	sys_halt();
}
