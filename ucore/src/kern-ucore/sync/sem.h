#ifndef __KERN_SYNC_SEM_H__
#define __KERN_SYNC_SEM_H__

#include <types.h>
#include <atomic.h>
#include <wait.h>
#include <spinlock.h>

typedef struct semaphore {
	int value;
	bool valid;
	atomic_t count;
	wait_queue_t wait_queue;
	spinlock_s lock;
#ifdef UCONFIG_BIONIC_LIBC
	uintptr_t addr;
#endif				//UCONFIG_BIONIC_LIBC
} semaphore_t;

// The sem_undo_t is used to permit semaphore manipulations that can be undone. If a process
// crashes after modifying a semaphore, the information held in the list is used to return the semaphore
// to its state prior to modification. The mechanism is useful when the crashed process has made changes
// after which processes waiting on the semaphore can no longer be woken. By undoing these actions (using
// the information in the sem_undo list), the semaphore can be returned to a consistent state, thus preventing
// deadlocks.
typedef struct sem_undo {
	semaphore_t *sem;
	list_entry_t semu_link;
} sem_undo_t;

#define le2semu(le, member)             \
    to_struct((le), sem_undo_t, member)

#ifdef UCONFIG_BIONIC_LIBC
#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#endif //UCONFIG_BIONIC_LIBC

typedef struct sem_queue {
	semaphore_t sem;
	atomic_t count;
	list_entry_t semu_list;
} sem_queue_t;

void sem_init(semaphore_t * sem, int value);
void up(semaphore_t * sem);
void down(semaphore_t * sem);
bool try_down(semaphore_t * sem);

sem_undo_t *semu_create(semaphore_t * sem, int value);
void semu_destroy(sem_undo_t * semu);

sem_queue_t *sem_queue_create(void);
void sem_queue_destroy(sem_queue_t * sem_queue);
int dup_sem_queue(sem_queue_t * to, sem_queue_t * from);
void exit_sem_queue(sem_queue_t * sem_queue);

int ipc_sem_init(int value);
int ipc_sem_post(sem_t sem_id);
int ipc_sem_wait(sem_t sem_id, unsigned int timeout);
int ipc_sem_free(sem_t sem_id);
int ipc_sem_get_value(sem_t sem_id, int *value_store);

#ifdef UCONFIG_BIONIC_LIBC
int do_futex(uintptr_t uaddr, int op, int val);
#endif //UCONFIG_BIONIC_LIBC

static inline int sem_count(semaphore_t * sem)
{
	return atomic_read(&(sem->count));
}

static inline void set_sem_count(semaphore_t * sem, int val)
{
	atomic_set(&(sem->count), val);
}

static inline int sem_count_inc(semaphore_t * sem)
{
	return atomic_add_return(&(sem->count), 1);
}

static inline int sem_count_dec(semaphore_t * sem)
{
	return atomic_sub_return(&(sem->count), 1);
}

static inline int sem_queue_count(sem_queue_t * sq)
{
	return atomic_read(&(sq->count));
}

static inline void set_sem_queue_count(sem_queue_t * sq, int val)
{
	atomic_set(&(sq->count), val);
}

static inline int sem_queue_count_inc(sem_queue_t * sq)
{
	return atomic_add_return(&(sq->count), 1);
}

static inline int sem_queue_count_dec(sem_queue_t * sq)
{
	return atomic_sub_return(&(sq->count), 1);
}

#endif /* !__KERN_SYNC_SEM_H__ */
