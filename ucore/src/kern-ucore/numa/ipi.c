/*
 * =====================================================================================
 *
 *       Filename:  ipi.c
 *
 *    Description: 
 *
 *        Version:  1.0
 *        Created:  05/27/2013 09:10:41 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include "mp.h"
#include <sysconf.h>

#define le2ipicall(le, member) \
	to_struct((le), struct ipi_call, member)

DEFINE_PERCPU_NOINIT(struct ipi_queue, ipi_queues);

void ipi_init(void)
{
	struct ipi_queue *q = get_cpu_ptr(ipi_queues);
	spinlock_init(&q->lock);
	list_init(&q->head);
	q->ipi_running = 0;
}

void do_ipicall(void)
{
	assert(!(read_rflags() & FL_IF));
	int id = myid();
	struct ipi_queue *myq = get_cpu_ptr(ipi_queues);
	while(1){
		spinlock_acquire(&myq->lock);
		if(list_empty(&myq->head)){
			myq->ipi_running = 0;
			spinlock_release(&myq->lock);
			break;
		}
		/* remove the first call */
		list_entry_t *first = list_next(&myq->head);
		list_del(first);
		/* hack */
		struct ipi_call *call = le2ipicall(first, call_queue_link[id]);
		myq->ipi_running = 1;
		spinlock_release(&myq->lock);
		assert(call->callback != NULL);
		call->callback(call);

		atomic_dec(&call->waiting);
	}
}

static void start_ipi(struct ipi_call* call, int cpuid)
{
	bool need_ipi = 0;
	struct ipi_queue *ipiq = per_cpu_ptr(ipi_queues, cpuid);
	spinlock_acquire(&ipiq->lock);
	if( !list_empty(&ipiq->head) && !ipiq->ipi_running)
		need_ipi = 1;
	list_add_before(&ipiq->head, &call->call_queue_link[cpuid]);
	spinlock_release(&ipiq->lock);
	if(need_ipi)
		fire_ipi_one(cpuid);
}

void ipi_run_on_cpu(const cpuset_t *cs, void *data, void (*cb)(struct ipi_call*))
{
	int ncpu = 0;
	int i;
	struct ipi_call call;
	bool interruptable = read_rflags() & FL_IF;
	int id = interruptable ? -1 : myid();
	memset(&call, 0, sizeof(call));
	for(i=0;i<sysconf.lcpu_count;i++)
		if(cpuset_test(cs, i))
			ncpu++;
	ncpu -= (!interruptable && cpuset_test(cs, id));
	atomic_set(&call.waiting, ncpu);
	call.callback = cb;
	call.private_data = data;
	for(i=0;i<sysconf.lcpu_count;i++){
		if(!cpuset_test(cs, i))
			continue;
		if(i == id)
			continue;
		start_ipi(&call, i);	
	}
	if(!interruptable && cpuset_test(cs, id))
		cb(&call);
	while(atomic_read(&call.waiting))
		nop_pause();

}

