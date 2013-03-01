#include <list.h>
#include <sync.h>
#include <proc.h>
#include <sched.h>
#include <stdio.h>
#include <assert.h>
#include <sched_RR.h>
#include <sched_MLFQ.h>
#include <kio.h>
#include <mp.h>
#include <trap.h>

#define current (pls_read(current))
#define idleproc (pls_read(idleproc))

/* For GDB ONLY - START */
/* Collect scheduling information to check how are the CPUs... */
#define SLICEPOOL_SIZE 21

static uint16_t sched_info_pid[PGSIZE / sizeof(uint16_t)];
static uint16_t sched_info_times[PGSIZE / sizeof(uint16_t)];
static int sched_info_head[8];
static int sched_slices[8][SLICEPOOL_SIZE];
int sched_collect_info = 1;

void db_sched(int lines)
{
	kprintf("\n");

	int lcpu_count = pls_read(lcpu_count);
	int i, j, k;
	/* Print a header */
	kprintf("        ");
	for (i = 0; i < lcpu_count; i++)
		kprintf("|    CPU%d    ", i);
	kprintf("\n");
	/* Print the table */
	for (i = 0; i < lines; i++) {
		kprintf(" %4d ", i);
		for (k = 0; k < lcpu_count; k++) {
			j = sched_info_head[k] - i;
			if (j < 0)
				j += PGSIZE / sizeof(uint16_t) / lcpu_count;
			kprintf("  %4d(%4d) ",
				sched_info_pid[j * lcpu_count + k],
				sched_info_times[j * lcpu_count + k]);
		}
		kprintf("\n");
	}
}

void db_time(uint16_t left, uint16_t right)
{
	kprintf("\n");

	int lcpu_count = pls_read(lcpu_count);
	int i, j;
	for (i = 0; i < lcpu_count; i++) {
		kprintf("On CPU%d: ", i);
		int sum = 0, total = PGSIZE / sizeof(uint16_t) / lcpu_count;
		for (j = 0; j < total; j++) {
			uint16_t pid = sched_info_pid[j * lcpu_count + i];
			if (pid >= left && pid <= right)
				sum += sched_info_times[j * lcpu_count + i];
		}
		kprintf("%4d", sum);
		sum = 0;
		for (j = left; j <= right; j++)
			sum += sched_slices[i][j % SLICEPOOL_SIZE];
		kprintf("(%4d)\n", sum);
	}
}

/* For GDB ONLY - END */

static list_entry_t timer_list;

static struct sched_class *sched_class;

static struct run_queue *rq;

static inline void sched_class_enqueue(struct proc_struct *proc)
{
	if (proc != idleproc) {
		sched_class->enqueue(rq, proc);
	}
}

static inline void sched_class_dequeue(struct proc_struct *proc)
{
	sched_class->dequeue(rq, proc);
}

static inline struct proc_struct *sched_class_pick_next(void)
{
	return sched_class->pick_next(rq);
}

static void sched_class_proc_tick(struct proc_struct *proc)
{
	if (proc != idleproc) {
		sched_class->proc_tick(rq, proc);
	} else {
		proc->need_resched = 1;
	}
}

static struct run_queue __rq[4];

void sched_init(void)
{
	list_init(&timer_list);

	rq = __rq;
	list_init(&(rq->rq_link));
	rq->max_time_slice = 8;

	int i;
	for (i = 1; i < sizeof(__rq) / sizeof(__rq[0]); i++) {
		list_add_before(&(rq->rq_link), &(__rq[i].rq_link));
		__rq[i].max_time_slice = rq->max_time_slice * (1 << i);
	}

#ifdef UCONFIG_SCHEDULER_MLFQ
	sched_class = &MLFQ_sched_class;
#else
	sched_class = &RR_sched_class;
#endif
	sched_class->init(rq);

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
				assert(proc->pid >= pls_read(lcpu_count));
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

#define MT_SUPPORT

void schedule(void)
{
	/* schedule in irq ctx is not allowed */
	assert(!ucore_in_interrupt());
	bool intr_flag;
	struct proc_struct *next;
#ifndef MT_SUPPORT
	list_entry_t head;
	int lapic_id = pls_read(lapic_id);
#endif

	local_intr_save(intr_flag);
	int lcpu_count = pls_read(lcpu_count);
	{
		current->need_resched = 0;
#ifndef MT_SUPPORT
		if (current->mm) {
			assert(current->mm->lapic == lapic_id);
			current->mm->lapic = -1;
		}
#endif
		if (current->state == PROC_RUNNABLE
		    && current->pid >= lcpu_count) {
			sched_class_enqueue(current);
		}
#ifndef MT_SUPPORT
		list_init(&head);
		while (1) {
			next = sched_class_pick_next();
			if (next != NULL)
				sched_class_dequeue(next);

			if (next && next->mm && next->mm->lapic != -1) {
				list_add(&head, &(next->run_link));
			} else {
				list_entry_t *cur;
				while ((cur = list_next(&head)) != &head) {
					list_del_init(cur);
					sched_class_enqueue(le2proc
							    (cur, run_link));
				}

				break;
			}
		}
#else
		next = sched_class_pick_next();
		if (next != NULL)
			sched_class_dequeue(next);
#endif /* !MT_SUPPORT */
		if (next == NULL) {
			next = idleproc;
		}
		next->runs++;
		/* Collect information here */
		if (sched_collect_info) {
			int lcpu_count = pls_read(lcpu_count);
			int lcpu_idx = pls_read(lcpu_idx);
			int loc = sched_info_head[lcpu_idx];
			int prev = sched_info_pid[loc * lcpu_count + lcpu_idx];
			if (next->pid == prev)
				sched_info_times[loc * lcpu_count + lcpu_idx]++;
			else {
				sched_info_head[lcpu_idx]++;
				if (sched_info_head[lcpu_idx] >=
				    PGSIZE / sizeof(uint16_t) / lcpu_count)
					sched_info_head[lcpu_idx] = 0;
				loc = sched_info_head[lcpu_idx];
				uint16_t prev_pid =
				    sched_info_pid[loc * lcpu_count + lcpu_idx];
				uint16_t prev_times =
				    sched_info_times[loc * lcpu_count +
						     lcpu_idx];
				if (prev_times > 0
				    && prev_pid >= lcpu_count + 2)
					sched_slices[lcpu_idx][prev_pid %
							       SLICEPOOL_SIZE]
					    += prev_times;
				sched_info_pid[loc * lcpu_count + lcpu_idx] =
				    next->pid;
				sched_info_times[loc * lcpu_count + lcpu_idx] =
				    1;
			}
		}
#ifndef MT_SUPPORT
		assert(!next->mm || next->mm->lapic == -1);
		if (next->mm)
			next->mm->lapic = lapic_id;
#endif
		if (next != current) {
#if 0
			kprintf("N %d to %d\n", current->pid, next->pid);
#endif
			proc_run(next);
		}
	}
	local_intr_restore(intr_flag);
}

void add_timer(timer_t * timer)
{
	bool intr_flag;
	local_intr_save(intr_flag);
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
	local_intr_restore(intr_flag);
}

void del_timer(timer_t * timer)
{
	bool intr_flag;
	local_intr_save(intr_flag);
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
	local_intr_restore(intr_flag);
}

void run_timer_list(void)
{
	bool intr_flag;
	local_intr_save(intr_flag);
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
					if (lt->function)
						(lt->function) (lt->data);
					del_timer(timer);
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

				del_timer(timer);
				if (le == &timer_list) {
					break;
				}
				timer = le2timer(le, timer_link);
			}
		}
		sched_class_proc_tick(current);
	}
	local_intr_restore(intr_flag);
}
