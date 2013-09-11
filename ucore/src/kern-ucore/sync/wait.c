#include <types.h>
#include <list.h>
#include <sync.h>
#include <wait.h>
#include <proc.h>
#include <sched.h>
#include <assert.h>

void wait_init(wait_t * wait, struct proc_struct *proc)
{
	wait->proc = proc;
	wait->wakeup_flags = WT_INTERRUPTED;
	list_init(&(wait->wait_link));
	spinlock_init(&wait->lock);
}

void wait_queue_init(wait_queue_t * queue)
{
	list_init(&(queue->wait_head));
	spinlock_init(&queue->lock);
}

/* assume intr has been disabled in the following API */
void wait_queue_add(wait_queue_t * queue, wait_t * wait)
{
	spinlock_acquire(&queue->lock);
	assert(list_empty(&(wait->wait_link)) && wait->proc != NULL);
	wait->wait_queue = queue;
	list_add_before(&(queue->wait_head), &(wait->wait_link));
	spinlock_release(&queue->lock);
}

void wait_queue_del(wait_queue_t * queue, wait_t * wait)
{
	spinlock_acquire(&queue->lock);
	assert(!list_empty(&(wait->wait_link)) && wait->wait_queue == queue);
	list_del_init(&(wait->wait_link));
	spinlock_release(&queue->lock);
}

wait_t *wait_queue_next(wait_queue_t * queue, wait_t * wait)
{
	spinlock_acquire(&queue->lock);
	assert(!list_empty(&(wait->wait_link)) && wait->wait_queue == queue);
	list_entry_t *le = list_next(&(wait->wait_link));
	if (le != &(queue->wait_head)) {
		spinlock_release(&queue->lock);
		return le2wait(le, wait_link);
	}
	spinlock_release(&queue->lock);
	return NULL;
}

wait_t *wait_queue_prev(wait_queue_t * queue, wait_t * wait)
{
	spinlock_acquire(&queue->lock);
	assert(!list_empty(&(wait->wait_link)) && wait->wait_queue == queue);
	list_entry_t *le = list_prev(&(wait->wait_link));
	if (le != &(queue->wait_head)) {
		spinlock_release(&queue->lock);
		return le2wait(le, wait_link);
	}
	spinlock_release(&queue->lock);
	return NULL;
}

wait_t *wait_queue_first(wait_queue_t * queue)
{
	spinlock_acquire(&queue->lock);
	list_entry_t *le = list_next(&(queue->wait_head));
	if (le != &(queue->wait_head)) {
		spinlock_release(&queue->lock);
		return le2wait(le, wait_link);
	}
	spinlock_release(&queue->lock);
	return NULL;
}

wait_t *wait_queue_last(wait_queue_t * queue)
{
	spinlock_acquire(&queue->lock);
	list_entry_t *le = list_prev(&(queue->wait_head));
	if (le != &(queue->wait_head)) {
		spinlock_release(&queue->lock);
		return le2wait(le, wait_link);
	}
	spinlock_release(&queue->lock);
	return NULL;
}

bool wait_queue_empty(wait_queue_t * queue)
{
	spinlock_acquire(&queue->lock);
	bool ret = list_empty(&(queue->wait_head));
	spinlock_release(&queue->lock);
	return ret;
}

bool wait_in_queue(wait_t * wait)
{
	if(!wait->wait_queue)
		return 0;
	spinlock_acquire(&wait->lock);
	bool ret = !list_empty(&(wait->wait_link));
	spinlock_release(&wait->lock);
	return ret;
}

void
wakeup_wait(wait_queue_t * queue, wait_t * wait, uint32_t wakeup_flags,
	    bool del)
{
	if (del) {
		wait_queue_del(queue, wait);
	}
	spinlock_acquire(&wait->lock);
	wait->wakeup_flags = wakeup_flags;
	spinlock_release(&wait->lock);
	wakeup_proc(wait->proc);
}

void wakeup_first(wait_queue_t * queue, uint32_t wakeup_flags, bool del)
{
	wait_t *wait;
	if ((wait = wait_queue_first(queue)) != NULL) {
		wakeup_wait(queue, wait, wakeup_flags, del);
	}
}

void wakeup_queue(wait_queue_t * queue, uint32_t wakeup_flags, bool del)
{
	wait_t *wait;
	if ((wait = wait_queue_first(queue)) != NULL) {
		if (del) {
			do {
				wakeup_wait(queue, wait, wakeup_flags, 1);
			} while ((wait = wait_queue_first(queue)) != NULL);
		} else {
			do {
				wakeup_wait(queue, wait, wakeup_flags, 0);
			} while ((wait = wait_queue_next(queue, wait)) != NULL);
		}
	}
}

void wait_current_set(wait_queue_t * queue, wait_t * wait, uint32_t wait_state)
{
	assert(current != NULL);
	wait_init(wait, current);
	current->state = PROC_SLEEPING;
	current->wait_state = wait_state;
	wait_queue_add(queue, wait);
}
