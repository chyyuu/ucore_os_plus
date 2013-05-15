#include <types.h>
#include <list.h>
#include <proc.h>
#include <assert.h>
#include <sched_mpRR.h>

static void MPRR_init(struct run_queue *rq)
{
	list_init(&(rq->run_list));
	rq->proc_num = 0;
}

static void MPRR_enqueue(struct run_queue *rq, struct proc_struct *proc)
{
	assert(list_empty(&(proc->run_link)));
	list_add_before(&(rq->run_list), &(proc->run_link));
	if (proc->time_slice == 0 || proc->time_slice > rq->max_time_slice) {
		proc->time_slice = rq->max_time_slice;
	}
	proc->rq = rq;
	rq->proc_num++;
}

static void MPRR_dequeue(struct run_queue *rq, struct proc_struct *proc)
{
	assert(!list_empty(&(proc->run_link)) && proc->rq == rq);
	list_del_init(&(proc->run_link));
	rq->proc_num--;
}

static struct proc_struct *MPRR_pick_next(struct run_queue *rq)
{
	list_entry_t *le = list_next(&(rq->run_list));
	if (le != &(rq->run_list)) {
		return le2proc(le, run_link);
	}
	return NULL;
}

static void MPRR_proc_tick(struct run_queue *rq, struct proc_struct *proc)
{
	if (proc->time_slice > 0) {
		proc->time_slice--;
	}
	if (proc->time_slice == 0) {
		proc->need_resched = 1;
	}
}

struct sched_class MPRR_sched_class = {
	.name = "MPRR_scheduler",
	.init = MPRR_init,
	.enqueue = MPRR_enqueue,
	.dequeue = MPRR_dequeue,
	.pick_next = MPRR_pick_next,
	.proc_tick = MPRR_proc_tick,
};
