#include <list.h>
#include <sync.h>
#include <proc.h>
#include <sched.h>
#include <stdio.h>
#include <assert.h>
#include <sched_RR.h>
#include <sched_MLFQ.h>
#include <sched_mpRR.h>
#include <kio.h>
#include <mp.h>
#include <trap.h>
#include <sysconf.h>
#include <spinlock.h>

/* we may use lock free list */
struct __timer_list_t{
	list_entry_t tl;
	spinlock_s lock;
};
static struct __timer_list_t __timer_list;
#define timer_list __timer_list.tl

static struct sched_class *sched_class;
static DEFINE_PERCPU_NOINIT(struct run_queue, runqueues);

//static struct run_queue *rq;

static inline void sched_class_enqueue(struct proc_struct *proc)
{
	if (proc != idleproc) {
		//TODO load balance
		struct run_queue *rq = get_cpu_ptr(runqueues);
		if(proc->flags & PF_PINCPU){
			assert(proc->cpu_affinity >= 0 
					&& proc->cpu_affinity < sysconf.lcpu_count);
			rq = per_cpu_ptr(runqueues, proc->cpu_affinity);
		}
		//XXX lock
		sched_class->enqueue(rq, proc);
	}
}

static inline void sched_class_dequeue(struct proc_struct *proc)
{
	struct run_queue *rq = get_cpu_ptr(runqueues);
	sched_class->dequeue(rq, proc);
}

static inline struct proc_struct *sched_class_pick_next(void)
{
	struct run_queue *rq = get_cpu_ptr(runqueues);
	return sched_class->pick_next(rq);
}

static void sched_class_proc_tick(struct proc_struct *proc)
{
	if (proc != idleproc) {
		struct run_queue *rq = get_cpu_ptr(runqueues);
		sched_class->proc_tick(rq, proc);
	} else {
		proc->need_resched = 1;
	}
}

//static struct run_queue __rq[NCPU];

void sched_init(void)
{
	list_init(&timer_list);

	//rq = __rq;
	//list_init(&(__rq[0].rq_link));
	struct run_queue *rq0 = get_cpu_ptr(runqueues);
	list_init(&(rq0->rq_link));
	rq0->max_time_slice = 8;

	int i;
	for (i = 1; i < sysconf.lcpu_count; i++) {
		struct run_queue *rqi = per_cpu_ptr(runqueues, i);
		list_add_before(&(rq0->rq_link), 
				&(rqi->rq_link));
		rqi->max_time_slice = rq0->max_time_slice;
	}

#ifdef UCONFIG_SCHEDULER_MLFQ
	sched_class = &MLFQ_sched_class;
#elif defined UCONFIG_SCHEDULER_RR
	sched_class = &RR_sched_class;
#else
	sched_class = &MPRR_sched_class;
#endif
	for (i = 0; i < sysconf.lcpu_count; i++) {
		struct run_queue *rqi = per_cpu_ptr(runqueues, i);
		sched_class->init(rqi);
	}

	kprintf("sched class: %s\n", sched_class->name);
}

void stop_proc(struct proc_struct *proc, uint32_t wait)
{
	bool intr_flag;
	local_intr_save(intr_flag);
	proc->state = PROC_SLEEPING;
	proc->wait_state = wait;
	if (!list_empty(&(proc->run_link))) {
		sched_class_dequeue(proc);
	}
	local_intr_restore(intr_flag);
}

void wakeup_proc(struct proc_struct *proc)
{
	assert(proc->state != PROC_ZOMBIE);
	bool intr_flag;
	local_intr_save(intr_flag);
	{
		if (proc->state != PROC_RUNNABLE) {
			proc->state = PROC_RUNNABLE;
			proc->wait_state = 0;
			if (proc != current) {
				assert(proc->pid >= sysconf.lcpu_count);
				sched_class_enqueue(proc);
			}
		} else {
			warn("wakeup runnable process.\n");
		}
	}
	local_intr_restore(intr_flag);
}

int try_to_wakeup(struct proc_struct *proc)
{
	assert(proc->state != PROC_ZOMBIE);
	int ret;
	bool intr_flag;
	local_intr_save(intr_flag);
	{
		if (proc->state != PROC_RUNNABLE) {
			proc->state = PROC_RUNNABLE;
			proc->wait_state = 0;
			if (proc != current) {
				sched_class_enqueue(proc);
			}
			ret = 1;
		} else {
			ret = 0;
		}
		struct proc_struct *next = proc;
		while ((next = next_thread(next)) != proc) {
			if (next->state == PROC_SLEEPING
			    && next->wait_state == WT_SIGNAL) {
				next->state = PROC_RUNNABLE;
				next->wait_state = 0;
				if (next != current) {
					sched_class_enqueue(next);
				}
			}
		}
	}
	local_intr_restore(intr_flag);
	return ret;
}

#include <vmm.h>

void schedule(void)
{
	/* schedule in irq ctx is not allowed */
	assert(!ucore_in_interrupt());
	bool intr_flag;
	struct proc_struct *next;

	local_intr_save(intr_flag);
	int lcpu_count = sysconf.lcpu_count;
	{
		current->need_resched = 0;
		if (current->state == PROC_RUNNABLE
		    && current->pid >= lcpu_count) {
			sched_class_enqueue(current);
		}

		next = sched_class_pick_next();
		if (next != NULL)
			sched_class_dequeue(next);
		else
			next = idleproc;
		next->runs++;
		if (next != current)
			proc_run(next);
	}
	local_intr_restore(intr_flag);
}

static void __add_timer(timer_t * timer)
{
	assert(timer->expires > 0 && timer->proc != NULL);
	assert(list_empty(&(timer->timer_link)));
	list_entry_t *le = list_next(&timer_list);
	while (le != &timer_list) {
		timer_t *next = le2timer(le, timer_link);
		if (timer->expires < next->expires) {
			next->expires -= timer->expires;
			break;
		}
		timer->expires -= next->expires;
		le = list_next(le);
	}
	list_add_before(le, &(timer->timer_link));
}

void add_timer(timer_t * timer)
{
	bool intr_flag;
	spin_lock_irqsave(&__timer_list.lock, intr_flag);
	{
		__add_timer(timer);
	}
	spin_unlock_irqrestore(&__timer_list.lock, intr_flag);
}

static void __del_timer(timer_t * timer)
{
	if (!list_empty(&(timer->timer_link))) {
		if (timer->expires != 0) {
			list_entry_t *le =
				list_next(&(timer->timer_link));
			if (le != &timer_list) {
				timer_t *next =
					le2timer(le, timer_link);
				next->expires += timer->expires;
			}
		}
		list_del_init(&(timer->timer_link));
	}
}

void del_timer(timer_t * timer)
{

	bool intr_flag;
	spin_lock_irqsave(&__timer_list.lock, intr_flag);
	{
		__del_timer(timer);
	}
	spin_unlock_irqrestore(&__timer_list.lock, intr_flag);
}

void run_timer_list(void)
{
	bool intr_flag;
	spin_lock_irqsave(&__timer_list.lock, intr_flag);
	{
		list_entry_t *le = list_next(&timer_list);
		if (le != &timer_list) {
			timer_t *timer = le2timer(le, timer_link);
			assert(timer->expires != 0);
			timer->expires--;
			while (timer->expires == 0) {
				le = list_next(le);
				if (__ucore_is_linux_timer(timer)) {
					struct __ucore_linux_timer *lt =
					    &(timer->linux_timer);

					spin_unlock_irqrestore(&__timer_list.lock, intr_flag);
					if (lt->function)
						(lt->function) (lt->data);
					spin_lock_irqsave(&__timer_list.lock, intr_flag);

					__del_timer(timer);
					kfree(timer);
					continue;
				}
				struct proc_struct *proc = timer->proc;
				if (proc->wait_state != 0) {
					assert(proc->wait_state &
					       WT_INTERRUPTED);
				} else {
					warn("process %d's wait_state == 0.\n",
					     proc->pid);
				}

				wakeup_proc(proc);

				__del_timer(timer);
				if (le == &timer_list) {
					break;
				}
				timer = le2timer(le, timer_link);
			}
		}
		sched_class_proc_tick(current);
	}
	spin_unlock_irqrestore(&__timer_list.lock, intr_flag);
}
