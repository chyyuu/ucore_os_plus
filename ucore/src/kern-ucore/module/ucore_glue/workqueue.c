/*
 * =====================================================================================
 *
 *       Filename:  workqueue.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/14/2012 11:39:33 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/signal.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/kthread.h>
#include <linux/hardirq.h>
#include <linux/mempolicy.h>
#include <linux/freezer.h>
#include <linux/kallsyms.h>
#include <linux/debug_locks.h>
#include <linux/lockdep.h>

#define _TODO_() printk(KERN_ALERT "TODO %s\n", __func__)

struct cpu_workqueue_struct {

	spinlock_t lock;

	struct list_head worklist;
	wait_queue_head_t more_work;
	struct work_struct *current_work;

	struct workqueue_struct *wq;
	struct task_struct *thread;
} ____cacheline_aligned;

/*
 * The externally visible workqueue abstraction is an array of
 * per-CPU workqueues:
 */
struct workqueue_struct {
	struct cpu_workqueue_struct *cpu_wq;
	struct list_head list;
	const char *name;
	int singlethread;
	int freezeable;		/* Freeze threads during suspend */
	int rt;
};

/* If it's single threaded, it isn't in the list of workqueues. */
static inline int is_wq_single_threaded(struct workqueue_struct *wq)
{
	return wq->singlethread;
}

/*
 * Set the workqueue on which a work item is to be run
 * - Must *only* be called if the pending flag is set
 */
static inline void set_wq_data(struct work_struct *work,
			       struct cpu_workqueue_struct *cwq)
{
	unsigned long new;

	BUG_ON(!work_pending(work));

	new = (unsigned long)cwq | (1UL << WORK_STRUCT_PENDING);
	new |= WORK_STRUCT_FLAG_MASK & *work_data_bits(work);
	atomic_long_set(&work->data, new);
}

static inline struct cpu_workqueue_struct *get_wq_data(struct work_struct *work)
{
	return (void *)(atomic_long_read(&work->data) &
			WORK_STRUCT_WQ_DATA_MASK);
}

static void insert_work(struct cpu_workqueue_struct *cwq,
			struct work_struct *work, struct list_head *head)
{
	//trace_workqueue_insertion(cwq->thread, work);
	//pr_debug("## insert_work\n\n");

	set_wq_data(work, cwq);
	/*
	 * Ensure that we get the right work->data if we see the
	 * result of list_add() below, see try_to_grab_pending().
	 */
	smp_wmb();
	list_add_tail(&work->entry, head);
	//wake_up(&cwq->more_work);
	__ucore_wakeup_by_pid(cwq->thread->pid);
}

static void __queue_work(struct cpu_workqueue_struct *cwq,
			 struct work_struct *work)
{
	unsigned long flags;

	spin_lock_irqsave(&cwq->lock, flags);
	insert_work(cwq, work, &cwq->worklist);
	spin_unlock_irqrestore(&cwq->lock, flags);
}

/**
 * queue_work_on - queue work on specific cpu
 * @cpu: CPU number to execute work on
 * @wq: workqueue to use
 * @work: work to queue
 *
 * Returns 0 if @work was already on a queue, non-zero otherwise.
 *
 * We queue the work to a specific CPU, the caller must ensure it
 * can't go away.
 */
int
queue_work_on(int cpu, struct workqueue_struct *wq, struct work_struct *work)
{
	int ret = 0;

	if (!test_and_set_bit(WORK_STRUCT_PENDING, work_data_bits(work))) {
		BUG_ON(!list_empty(&work->entry));
		__queue_work(wq->cpu_wq, work);
		ret = 1;
	}
	return ret;
}

EXPORT_SYMBOL_GPL(queue_work_on);

/**
 * queue_work - queue work on a workqueue
 * @wq: workqueue to use
 * @work: work to queue
 *
 * Returns 0 if @work was already on a queue, non-zero otherwise.
 *
 * We queue the work to the CPU on which it was submitted, but if the CPU dies
 * it can be processed by another CPU.
 */
int queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
	int ret;

	ret = queue_work_on(0, wq, work);

	return ret;
}

EXPORT_SYMBOL_GPL(queue_work);

int queue_delayed_work(struct workqueue_struct *wq,
		       struct delayed_work *dwork, unsigned long delay)
{
	if (delay == 0)
		return queue_work(wq, &dwork->work);

	return queue_delayed_work_on(-1, wq, dwork, delay);
}

static void delayed_work_timer_fn(unsigned long __data)
{
	pr_debug("HERE delayed_work_timer_fn\n");
	struct delayed_work *dwork = (struct delayed_work *)__data;
	struct cpu_workqueue_struct *cwq = get_wq_data(&dwork->work);
	struct workqueue_struct *wq = cwq->wq;

	__queue_work(wq->cpu_wq, &dwork->work);
}

/**
 * queue_delayed_work_on - queue work on specific CPU after delay
 * @cpu: CPU number to execute work on
 * @wq: workqueue to use
 * @dwork: work to queue
 * @delay: number of jiffies to wait before queueing
 *
 * Returns 0 if @work was already on a queue, non-zero otherwise.
 */
int queue_delayed_work_on(int cpu, struct workqueue_struct *wq,
			  struct delayed_work *dwork, unsigned long delay)
{
	int ret = 0;
	struct timer_list *timer = &dwork->timer;
	struct work_struct *work = &dwork->work;

	if (!test_and_set_bit(WORK_STRUCT_PENDING, work_data_bits(work))) {
		BUG_ON(timer_pending(timer));
		BUG_ON(!list_empty(&work->entry));

		timer_stats_timer_set_start_info(&dwork->timer);

		/* This stores cwq for the moment, for the timer_fn */
		set_wq_data(work, wq->cpu_wq);
		timer->expires = jiffies + delay;
		timer->data = (unsigned long)dwork;
		timer->function = delayed_work_timer_fn;

		linux_add_timer(timer);
		ret = 1;
	}
	return ret;
}

EXPORT_SYMBOL_GPL(queue_delayed_work_on);

void flush_workqueue(struct workqueue_struct *wq)
{
	_TODO_();
}

void add_wait_queue(wait_queue_head_t * q, wait_queue_t * wait)
{
	_TODO_();
}

/**
 * destroy_workqueue - safely terminate a workqueue
 * @wq: target workqueue
 *
 * Safely destroy a workqueue. All work currently pending will be done first.
898  */
void destroy_workqueue(struct workqueue_struct *wq)
{
	_TODO_();
}

void remove_wait_queue(wait_queue_head_t * q, wait_queue_t * wait)
{
	_TODO_();
}

EXPORT_SYMBOL(remove_wait_queue);

static void run_workqueue(struct cpu_workqueue_struct *cwq)
{
	spin_lock_irq(&cwq->lock);
	while (!list_empty(&cwq->worklist)) {
		struct work_struct *work = list_entry(cwq->worklist.next,
						      struct work_struct,
						      entry);
		work_func_t f = work->func;
		cwq->current_work = work;
		list_del_init(cwq->worklist.next);
		spin_unlock_irq(&cwq->lock);

		BUG_ON(get_wq_data(work) != cwq);
		work_clear_pending(work);
		f(work);

		spin_lock_irq(&cwq->lock);
		cwq->current_work = NULL;
	}
	spin_unlock_irq(&cwq->lock);
}

static int worker_thread(void *__cwq)
{
	struct cpu_workqueue_struct *cwq = __cwq;
	BUG_ON(cwq->thread->pid == 0);
	for (;;) {
		//run_workqueue(cwq);
		run_workqueue(cwq);
		//extern do_sleep(int);
		//do_sleep(3);
		__ucore_wait_self();
	}

	return 0;
}

static int create_workqueue_thread(struct cpu_workqueue_struct *cwq, int cpu)
{
	struct workqueue_struct *wq = cwq->wq;
	const char *fmt = is_wq_single_threaded(wq) ? "%s" : "%s/%d";
	struct task_struct *p =
	    kthread_create(worker_thread, cwq, fmt, wq->name, cpu);
	if (IS_ERR(p))
		return PTR_ERR(p);
	cwq->thread = p;
	return 0;
}

static struct cpu_workqueue_struct *init_cpu_workqueue(struct workqueue_struct
						       *wq, int cpu)
{
	struct cpu_workqueue_struct *cwq = wq->cpu_wq;

	cwq->wq = wq;
	spin_lock_init(&cwq->lock);
	INIT_LIST_HEAD(&cwq->worklist);
	init_waitqueue_head(&cwq->more_work);

	return cwq;
}

struct workqueue_struct *__create_workqueue_key(const char *name,
						int singlethread,
						int freezeable,
						int rt,
						struct lock_class_key *key,
						const char *lock_name)
{
	struct workqueue_struct *wq = NULL;
	int err = 0;
	struct cpu_workqueue_struct *cwq;
	if (!singlethread) {
		pr_debug("__create_workqueue_key only support singlethread\n");
		return NULL;
	}
	wq = kzalloc(sizeof(struct workqueue_struct), GFP_KERNEL);
	if (!wq)
		return NULL;
	wq->cpu_wq = kzalloc(sizeof(struct cpu_workqueue_struct), GFP_KERNEL);
	if (!wq->cpu_wq) {
		kfree(wq);
		return NULL;
	}
	wq->cpu_wq->wq = wq;

	wq->name = name;
	wq->singlethread = singlethread;
	wq->freezeable = freezeable;
	wq->rt = rt;
	INIT_LIST_HEAD(&wq->list);
	cwq = init_cpu_workqueue(wq, 0);
	err = create_workqueue_thread(wq->cpu_wq, 0);
	if (err)
		goto create_thread_failed;

	_TODO_();
	return wq;
create_thread_failed:
	kfree(wq->cpu_wq);
	kfree(wq);
	return NULL;
}

/* ucore */
static struct workqueue_struct *__dde_def_workqueue;
int init_dde_workqueue()
{
	__dde_def_workqueue = create_singlethread_workqueue("kworkqueue");
	if (!__dde_def_workqueue)
		return -ENOMEM;
	return 0;
}

int schedule_work(struct work_struct *work)
{
	return queue_delayed_work(__dde_def_workqueue, work, 0);
}
