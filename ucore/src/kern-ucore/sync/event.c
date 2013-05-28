#include <types.h>
#include <wait.h>
#include <proc.h>
#include <vmm.h>
#include <ipc.h>
#include <sync.h>
#include <assert.h>
#include <error.h>
#include <clock.h>
#include <event.h>

void event_box_init(event_t * event_box)
{
	wait_queue_init(&(event_box->wait_queue));
}

static uint32_t send_event(struct proc_struct *proc, timer_t * timer)
{
	bool intr_flag;
	local_intr_save(intr_flag);
	wait_t __wait, *wait = &__wait;
	wait_queue_t *wait_queue = &(proc->event_box.wait_queue);
	wait_current_set(wait_queue, wait, WT_EVENT_SEND);
	ipc_add_timer(timer);
	local_intr_restore(intr_flag);

	schedule();

	local_intr_save(intr_flag);
	ipc_del_timer(timer);
	wait_current_del(wait_queue, wait);
	local_intr_restore(intr_flag);

	if (wait->wakeup_flags != WT_EVENT_SEND) {
		return wait->wakeup_flags;
	}
	return 0;
}

int ipc_event_send(int pid, int event, unsigned int timeout)
{
	struct proc_struct *proc;
	if ((proc = find_proc(pid)) == NULL || proc->state == PROC_ZOMBIE) {
		return -E_INVAL;
	}
	if (proc == current || proc == idleproc || proc == initproc) {
		return -E_INVAL;
	}
#ifdef UCONFIG_SWAP
	if(proc == kswapd)
		return -E_INVAL;
#endif
	if (proc->wait_state == WT_EVENT_RECV) {
		wakeup_proc(proc);
	}
	current->event_box.event = event;

	unsigned long saved_ticks;
	timer_t __timer, *timer =
	    ipc_timer_init(timeout, &saved_ticks, &__timer);

	uint32_t flags;
	if ((flags = send_event(proc, timer)) == 0) {
		return 0;
	}
	assert(flags == WT_INTERRUPTED);
	return ipc_check_timeout(timeout, saved_ticks);
}

static int recv_event(int *pid_store, int *event_store, timer_t * timer)
{
	bool intr_flag;
	local_intr_save(intr_flag);
	wait_queue_t *wait_queue = &(current->event_box.wait_queue);
	if (wait_queue_empty(wait_queue)) {
		current->state = PROC_SLEEPING;
		current->wait_state = WT_EVENT_RECV;
		ipc_add_timer(timer);
		local_intr_restore(intr_flag);

		schedule();

		local_intr_save(intr_flag);
		ipc_del_timer(timer);
	}

	int ret = -1;

	wait_t *wait;
	if ((wait = wait_queue_first(wait_queue)) != NULL) {
		struct proc_struct *proc = wait->proc;
		*pid_store = proc->pid, *event_store =
		    proc->event_box.event, ret = 0;
		wakeup_wait(wait_queue, wait, WT_EVENT_SEND, 1);
	}
	local_intr_restore(intr_flag);
	return ret;
}

int ipc_event_recv(int *pid_store, int *event_store, unsigned int timeout)
{
	if (event_store == NULL) {
		return -E_INVAL;
	}

	struct mm_struct *mm = current->mm;
	if (pid_store != NULL) {
		if (!user_mem_check(mm, (uintptr_t) pid_store, sizeof(int), 1)) {
			return -E_INVAL;
		}
	}
	if (!user_mem_check(mm, (uintptr_t) event_store, sizeof(int), 1)) {
		return -E_INVAL;
	}

	unsigned long saved_ticks;
	timer_t __timer, *timer =
	    ipc_timer_init(timeout, &saved_ticks, &__timer);

	int pid, event, ret;
	if ((ret = recv_event(&pid, &event, timer)) == 0) {
		lock_mm(mm);
		{
			ret = -E_INVAL;
			if (pid_store == NULL
			    || copy_to_user(mm, pid_store, &pid, sizeof(int))) {
				if (copy_to_user
				    (mm, event_store, &event, sizeof(int))) {
					ret = 0;
				}
			}
		}
		unlock_mm(mm);
		return ret;
	}
	return ipc_check_timeout(timeout, saved_ticks);
}
