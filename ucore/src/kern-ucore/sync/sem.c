#include <types.h>
#include <wait.h>
#include <atomic.h>
#include <slab.h>
#include <sem.h>
#include <vmm.h>
#include <ipc.h>
#include <proc.h>
#include <sync.h>
#include <assert.h>
#include <error.h>
#include <clock.h>

/*  ------------- semaphore mechanism design&implementation -------------
  ucore offers two kinds of semaphores: Kernel semaphores, which are used by kernel control paths; 
System V IPC semaphores, which are used by User Mode processes.A kernel control path tries to 
acquire a busy resource protected by a kernel semaphore, the corresponding process is suspended. 
It becomes runnable again when the resource is released.
V operation:
  When a process wishes to release a semaphore lock, it invokes the __up(Kern_SEM or USer_SEM) 
function.The __up( ) function first checks whether its wait_queue(the waiting process list) is NULL. 
If wait_queue is NULL, then increases the count field of the sem semaphore (sem->value ++); else 
wakeup a waiting process and delete this proces from wait_queue.
P operation:
  Conversely, when a process wishes to acquire a semaphore lock, it invokes the __down(Kern_SEM or 
USer_SEM) function. The __down( ) function first checks whether semaphore value is negative or zero.
If the value >0, then decreases the count field of the *sem semaphore(sem->value--);else the current 
process must be suspended, so put current process in semaphore's wait queue. Essentially, the __down( ) 
function changes the state of the current process from PROC_RUNNABLE to PROC_SLEEPING.

Special Case&Operation for User Semaphore 
If a process crashes after modifying a user semaphore,then processes waiting on the user semaphore can 
no longer be woken. ucore uses sem_undo struct to record the information held in the list to return the 
semaphore to its state prior to modification By undoing these actions (using the information in the 
sem_undo list), the user semaphore can be returned to a consistent state, thus preventing deadlocks.

IMPLEMENTATION NOTE:
Now, ucore didn't implementation 'Special Case&Operation for User Semaphore' in sem_destroy function. :(

*/

#define VALID_SEMID(sem_id)                     \
    ((uintptr_t)(sem_id) < (uintptr_t)(sem_id) + PBASE)

#define semid2sem(sem_id)                       \
    ((semaphore_t *)((uintptr_t)(sem_id) + PBASE))

#define sem2semid(sem)                          \
    ((sem_t)((uintptr_t)(sem) - PBASE))

void sem_init(semaphore_t * sem, int value)
{
	sem->value = value;
	sem->valid = 1;
	spinlock_init(&sem->lock);
#ifdef UCONFIG_BIONIC_LIBC
	sem->addr = 0;		//-1 : // Not for futex
#endif //UCONFIG_BIONIC_LIBC
	set_sem_count(sem, 0);
	wait_queue_init(&(sem->wait_queue));
}

#ifdef UCONFIG_BIONIC_LIBC
void sem_init_with_address(semaphore_t * sem, uintptr_t addr, int value)
{
	sem->value = value;
	sem->addr = addr;
	sem->valid = 1;
	spinlock_init(&sem->lock);
	set_sem_count(sem, 0);
	wait_queue_init(&(sem->wait_queue));
}
#endif //UCONFIG_BIONIC_LIBC

static void
    __attribute__ ((noinline)) __up(semaphore_t * sem, uint32_t wait_state)
{
	assert(sem->valid);
	bool intr_flag;
	spin_lock_irqsave(&sem->lock, intr_flag);
	{
		wait_t *wait;
		if ((wait = wait_queue_first(&(sem->wait_queue))) == NULL) {
			sem->value++;
		} else {
			assert(wait->proc->wait_state == wait_state);
			wakeup_wait(&(sem->wait_queue), wait, wait_state, 1);
		}
	}
	spin_unlock_irqrestore(&sem->lock, intr_flag);
}

static uint32_t
    __attribute__ ((noinline)) __down(semaphore_t * sem, uint32_t wait_state,
				      timer_t * timer)
{
	assert(sem->valid);
	bool intr_flag;
	spin_lock_irqsave(&sem->lock, intr_flag);
	if (sem->value > 0) {
		sem->value--;
		spin_unlock_irqrestore(&sem->lock, intr_flag);
		return 0;
	}
	wait_t __wait, *wait = &__wait;
	wait_current_set(&(sem->wait_queue), wait, wait_state);
	ipc_add_timer(timer);
	spin_unlock_irqrestore(&sem->lock, intr_flag);

	schedule();

	spin_lock_irqsave(&sem->lock, intr_flag);
	ipc_del_timer(timer);
	wait_current_del(&(sem->wait_queue), wait);
	spin_unlock_irqrestore(&sem->lock, intr_flag);

	if (wait->wakeup_flags != wait_state) {
		return wait->wakeup_flags;
	}
	return 0;
}

void up(semaphore_t * sem)
{
	__up(sem, WT_KSEM);
}

void down(semaphore_t * sem)
{
	uint32_t flags = __down(sem, WT_KSEM, NULL);
	assert(flags == 0);
}

bool try_down(semaphore_t * sem)
{
	bool intr_flag, ret = 0;
	spin_lock_irqsave(&sem->lock, intr_flag);
	if (sem->value > 0) {
		sem->value--, ret = 1;
	}
	spin_unlock_irqrestore(&sem->lock, intr_flag);
	return ret;
}

static int usem_up(semaphore_t * sem)
{
	__up(sem, WT_USEM);
	return 0;
}

static int usem_down(semaphore_t * sem, unsigned int timeout)
{
	unsigned long saved_ticks;
	timer_t __timer, *timer =
	    ipc_timer_init(timeout, &saved_ticks, &__timer);

	uint32_t flags;
	if ((flags = __down(sem, WT_USEM, timer)) == 0) {
		return 0;
	}
	assert(flags == WT_INTERRUPTED);
	return ipc_check_timeout(timeout, saved_ticks);
}

sem_queue_t *sem_queue_create(void)
{
	sem_queue_t *sem_queue;
	if ((sem_queue = kmalloc(sizeof(sem_queue_t))) != NULL) {
		sem_init(&(sem_queue->sem), 1);
		set_sem_queue_count(sem_queue, 0);
		list_init(&(sem_queue->semu_list));
	}
	return sem_queue;
}

void sem_queue_destroy(sem_queue_t * sem_queue)
{
	kfree(sem_queue);
}

#ifdef UCONFIG_BIONIC_LIBC
sem_undo_t *semu_create_with_address(semaphore_t * sem, uintptr_t addr,
				     int value)
{
	sem_undo_t *semu;
	if ((semu = kmalloc(sizeof(sem_undo_t))) != NULL) {
		if (sem == NULL && (sem = kmalloc(sizeof(semaphore_t))) != NULL) {
			sem_init_with_address(sem, addr, value);
		}
		if (sem != NULL) {
			sem_count_inc(sem);
			semu->sem = sem;
			return semu;
		}
		kfree(semu);
	}
	return NULL;
}
#endif //UCONFIG_BIONIC_LIBC

sem_undo_t *semu_create(semaphore_t * sem, int value)
{
	sem_undo_t *semu;
	if ((semu = kmalloc(sizeof(sem_undo_t))) != NULL) {
		if (sem == NULL && (sem = kmalloc(sizeof(semaphore_t))) != NULL) {
			sem_init(sem, value);
		}
		if (sem != NULL) {
			sem_count_inc(sem);
			semu->sem = sem;
			return semu;
		}
		kfree(semu);
	}
	return NULL;
}

void semu_destroy(sem_undo_t * semu)
{
	if (sem_count_dec(semu->sem) == 0) {
		kfree(semu->sem);
	}
	kfree(semu);
}

int dup_sem_queue(sem_queue_t * to, sem_queue_t * from)
{
	assert(to != NULL && from != NULL);
	list_entry_t *list = &(from->semu_list), *le = list;
	while ((le = list_next(le)) != list) {
		sem_undo_t *semu = le2semu(le, semu_link);
		if (semu->sem->valid) {
			if ((semu = semu_create(semu->sem, 0)) == NULL) {
				return -E_NO_MEM;
			}
			list_add(&(to->semu_list), &(semu->semu_link));
		}
	}
	return 0;
}

void exit_sem_queue(sem_queue_t * sem_queue)
{
	assert(sem_queue != NULL && sem_queue_count(sem_queue) == 0);
	list_entry_t *list = &(sem_queue->semu_list), *le = list;
	while ((le = list_next(list)) != list) {
		list_del(le);
		semu_destroy(le2semu(le, semu_link));
	}
}

static sem_undo_t *semu_list_search(list_entry_t * list, sem_t sem_id)
{
	if (VALID_SEMID(sem_id)) {
		semaphore_t *sem = semid2sem(sem_id);
		list_entry_t *le = list;
		while ((le = list_next(le)) != list) {
			sem_undo_t *semu = le2semu(le, semu_link);
			if (semu->sem == sem) {
				list_del(le);
				if (sem->valid) {
					list_add_after(list, le);
					return semu;
				} else {
					semu_destroy(semu);
					return NULL;
				}
			}
		}
	}
	return NULL;
}

#ifdef UCONFIG_BIONIC_LIBC
static int semu_search_with_addr(list_entry_t * list, uintptr_t addr)
{
	list_entry_t *le = list;
	while ((le = list_next(le)) != list) {
		sem_undo_t *semu = le2semu(le, semu_link);
		if (semu->sem->addr == addr)
			return sem2semid(semu->sem);
	}
	return -1;
}

static int
ipc_sem_find_or_init_with_address(uintptr_t addr, int value, int create)
{
	assert(current->sem_queue != NULL);

	sem_queue_t *sem_queue = current->sem_queue;
	down(&(sem_queue->sem));
	sem_t sem_id = semu_search_with_addr(&(sem_queue->semu_list), addr);
	up(&(sem_queue->sem));
	if (sem_id != -1)
		return sem_id;

	if (!create)
		return -E_NO_MEM;
	sem_undo_t *semu;
	if ((semu = semu_create_with_address(NULL, addr, value)) == NULL) {
		return -E_NO_MEM;
	}

	down(&(sem_queue->sem));
	list_add_after(&(sem_queue->semu_list), &(semu->semu_link));
	up(&(sem_queue->sem));
	return sem2semid(semu->sem);
}
#endif //UCONFIG_BIONIC_LIBC

int ipc_sem_init(int value)
{
	assert(current->sem_queue != NULL);

	sem_undo_t *semu;
	if ((semu = semu_create(NULL, value)) == NULL) {
		return -E_NO_MEM;
	}

	sem_queue_t *sem_queue = current->sem_queue;
	down(&(sem_queue->sem));
	list_add_after(&(sem_queue->semu_list), &(semu->semu_link));
	up(&(sem_queue->sem));
	return sem2semid(semu->sem);
}

int ipc_sem_post(sem_t sem_id)
{
	assert(current->sem_queue != NULL);

	sem_undo_t *semu;
	sem_queue_t *sem_queue = current->sem_queue;
	down(&(sem_queue->sem));
	semu = semu_list_search(&(sem_queue->semu_list), sem_id);
	up(&(sem_queue->sem));
	if (semu != NULL) {
		return usem_up(semu->sem);
	}
	return -E_INVAL;
}

#ifdef UCONFIG_BIONIC_LIBC
int ipc_sem_post_max(sem_t sem_id, int max)
{
	assert(current->sem_queue != NULL);

	sem_undo_t *semu;
	sem_queue_t *sem_queue = current->sem_queue;
	down(&(sem_queue->sem));
	semu = semu_list_search(&(sem_queue->semu_list), sem_id);
	up(&(sem_queue->sem));
	if (semu != NULL) {
		int i;
		int ret = 0;
		for (i = 0; i < max; ++i) {
			if (wait_queue_empty(&(semu->sem->wait_queue)))
				break;
			usem_up(semu->sem);
			++ret;
		}
		return ret;
	}
	return -E_INVAL;
}
#endif //UCONFIG_BIONIC_LIBC

int ipc_sem_wait(sem_t sem_id, unsigned int timeout)
{
	assert(current->sem_queue != NULL);

	sem_undo_t *semu;
	sem_queue_t *sem_queue = current->sem_queue;
	down(&(sem_queue->sem));
	semu = semu_list_search(&(sem_queue->semu_list), sem_id);
	up(&(sem_queue->sem));
	if (semu != NULL) {
		return usem_down(semu->sem, timeout);
	}
	return -E_INVAL;
}

int ipc_sem_free(sem_t sem_id)
{
	assert(current->sem_queue != NULL);

	sem_undo_t *semu;
	sem_queue_t *sem_queue = current->sem_queue;
	down(&(sem_queue->sem));
	semu = semu_list_search(&(sem_queue->semu_list), sem_id);
	up(&(sem_queue->sem));

	int ret = -E_INVAL;
	if (semu != NULL) {
		bool intr_flag;
		local_intr_save(intr_flag);
		{
			semaphore_t *sem = semu->sem;
			sem->valid = 0, ret = 0;
			wakeup_queue(&(sem->wait_queue), WT_INTERRUPTED, 1);
		}
		local_intr_restore(intr_flag);
	}
	return ret;
}

int ipc_sem_get_value(sem_t sem_id, int *value_store)
{
	assert(current->sem_queue != NULL);
	if (value_store == NULL) {
		return -E_INVAL;
	}

	struct mm_struct *mm = current->mm;
	if (!user_mem_check(mm, (uintptr_t) value_store, sizeof(int), 1)) {
		return -E_INVAL;
	}

	sem_undo_t *semu;
	sem_queue_t *sem_queue = current->sem_queue;
	down(&(sem_queue->sem));
	semu = semu_list_search(&(sem_queue->semu_list), sem_id);
	up(&(sem_queue->sem));

	int ret = -E_INVAL;
	if (semu != NULL) {
		int value = semu->sem->value;
		lock_mm(mm);
		{
			if (copy_to_user(mm, value_store, &value, sizeof(int))) {
				ret = 0;
			}
		}
		unlock_mm(mm);
	}
	return ret;
}

#ifdef UCONFIG_BIONIC_LIBC
int do_futex(uintptr_t uaddr, int op, int val)
{
	int ret = 0;
	if (FUTEX_WAIT == op) {
		if (*((int *)uaddr) != val) {
			panic
			    ("FUTEX_WAIT: unexpected val, *uaddr = %d, val = %d\n",
			     *((int *)uaddr), val);
		}
		sem_t sem_id = ipc_sem_find_or_init_with_address(uaddr, 0, 1);
		ret = ipc_sem_wait(sem_id, 100000);
	} else if (FUTEX_WAKE == op) {
		sem_t sem_id = ipc_sem_find_or_init_with_address(uaddr, val, 0);
		if (-E_NO_MEM == sem_id) {
		} else {
			ret = ipc_sem_post_max(sem_id, val);
		}
	} else {
		panic("unexpected futex op: %d\n", op);
	}
}
#endif //UCONFIG_BIONIC_LIBC
