/*
 * =====================================================================================
 *
 *       Filename:  refcache.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/15/2013 10:00:44 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
/* Reference: RadixVM: Scalable address spaces for multithreaded applications */

#include "refcache.h"
#include <sysconf.h>
#include <kio.h>
#include <wait.h>
#include <mp.h>
#include <sync.h>
#include <proc.h>
#include <string.h>

//#define __REFCACHE_TEST
#if 0
#define REFCACHE_DEBUG(...) kprintf(__VA_ARGS__)
#else
#define REFCACHE_DEBUG(...) 
#endif

DEFINE_PERCPU_NOINIT(struct refcache, refcaches);

static atomic_t global_epoch;
static atomic_t global_epoch_left;

#ifdef __REFCACHE_TEST
static int __evict_counter = 0;
static struct refobj __test_obj;
static void test_on_zero(struct refobj* obj){
	kprintf("CPU%d: free %p, evict %d\n", myid(), obj, __evict_counter);
}
#endif

void refcache_init(void)
{
	kprintf("refcache_init()\n");
	int i, j;
	for(i=0;i<sysconf.lcpu_count;i++){
		struct refcache *r = per_cpu_ptr(refcaches, i);
		assert(r!=NULL);
		list_init(&r->review_head);
		for(j=0;j<REF_CACHE_SLOT;j++)
			memset(&r->way[i], 0, sizeof(struct refway));
	}
	atomic_set(&global_epoch, 1);
	atomic_set(&global_epoch_left, sysconf.lcpu_count);
#ifdef __REFCACHE_TEST
	refcache_refobj_init(&__test_obj);
	set_refobj_callback(&__test_obj, test_on_zero);
	refcache_refobj_inc(&__test_obj);
#endif
}

static inline unsigned int __hash_refobj(struct refobj *obj)
{
	return hash32((size_t)obj >> 2, REF_CACHE_SLOT_SHIFT);
}

void refcache_refobj_init(struct refobj *obj)
{
	assert(obj!=NULL);
	memset(obj, 0, sizeof(struct refobj));
	list_init(&obj->review_link);
	spinlock_init(&obj->lock);
}

/* intr should be disabled */
static void __evict(struct refway *way)
{
	struct refobj *obj = way->obj;
	struct refcache * cache = get_cpu_ptr(refcaches);
	assert(obj != NULL);
	assert(way->delta != 0);
	spinlock_acquire(&obj->lock);
	obj->refcount += way->delta;
#ifdef __REFCACHE_TEST
	__evict_counter++;
#endif
	if(obj->refcount == 0){
		if(obj->review_epoch == 0){
			obj->flag &= ~REFOBJ_FLAG_DIRTY;
			/* 3 is always OK */
			obj->review_epoch = cache->local_epoch + 3;
			list_add_before(&(cache->review_head), 
					&(obj->review_link));
		}else{
			obj->flag |= REFOBJ_FLAG_DIRTY;
		}
	}
	spinlock_release(&obj->lock);
	way->obj = NULL;
}



static inline struct refway * __get_my_way(struct refobj* obj)
{
	struct refcache *r = get_cpu_ptr(refcaches);
	struct refway* way = &r->way[__hash_refobj(obj)];
	if(way->obj!= NULL && way->obj != obj){
		__evict(way);
	}
	if(way->obj == NULL){
		way->obj = obj;
		way->delta = 0;
	}
	return way;
}

void refcache_refobj_inc(struct refobj *obj)
{
	bool intr_flag;
	local_intr_save(intr_flag);
	struct refway *way = __get_my_way(obj);
	way->delta ++;
	local_intr_restore(intr_flag);
}

void refcache_refobj_dec(struct refobj *obj)
{
	bool intr_flag;
	local_intr_save(intr_flag);
	struct refway *way = __get_my_way(obj);
	way->delta --;
	local_intr_restore(intr_flag);
}
#define le2refobj(le, member) \
	to_struct((le), struct refobj, member)
static void flush(void)
{
	bool intr_flag;
	int i;
	local_intr_save(intr_flag);
	struct refcache *r = get_cpu_ptr(refcaches);
	int ge = atomic_read(&global_epoch);
	int nflush = 0;
	if(ge == r->local_epoch)
		goto done;
	r->local_epoch = ge;
	for(i=0;i<REF_CACHE_SLOT;i++){
		struct refway *way = r->way + i;
		if(!way->obj)
			continue;
		if(way->delta != 0){
			REFCACHE_DEBUG("RC %d %d\n", myid(), way->delta);
			nflush ++;
			__evict(way);
		}
		way->obj = NULL;
	}

	if(atomic_dec_test_zero(&global_epoch_left)){
		atomic_add(&global_epoch_left, sysconf.lcpu_count);
		atomic_inc(&global_epoch);
	}
done:
	local_intr_restore(intr_flag);
	REFCACHE_DEBUG("cpu%d flushed %d\n", myid(), nflush);
}

static void review(void)
{
	bool intr_flag;
	local_intr_save(intr_flag);
	struct refcache *r = get_cpu_ptr(refcaches);
	list_entry_t *list = &r->review_head, *le = list;
	int epoch = atomic_read(&global_epoch);
	int nfree = 0, ntotal = 0;
	while((le = list_next(le)) != list){
		struct refobj *obj = le2refobj(le, review_link);
		if(obj->review_epoch > epoch)
			continue;
		spinlock_acquire(&obj->lock);
		list_del(&obj->review_link);
		obj->review_epoch = 0;
		ntotal ++;
		if(obj->refcount != 0){
			/* nothing */
		}else if(obj->flag & REFOBJ_FLAG_DIRTY){
			obj->flag &= ~REFOBJ_FLAG_DIRTY;
			obj->review_epoch = epoch + 2;
			list_add_before(&r->review_head, &obj->review_link);
		}else{
			if(obj->onzero)
				obj->onzero(obj);
			nfree++;
		}
		spinlock_release(&obj->lock);
	}

	local_intr_restore(intr_flag);
	REFCACHE_DEBUG("%d: e %d, ntotal %d, nfree %d\n", myid(), epoch, ntotal, nfree);


}

void refcache_tick(void)
{
	flush();
	review();
}


int krefcache_cleaner(void *arg)
{
	int id = myid();
	kprintf("start refcache test %d %d\n", id, current->cpu_affinity);
	assert(current->cpu_affinity == id);
	int i = 0;
	while(1){
#ifdef __REFCACHE_TEST
#define TEST_ROUND 19
		/* basic test */
		if(i < TEST_ROUND){
			refcache_refobj_inc(&__test_obj);
		}
		if(i >= 10 && i<10+TEST_ROUND){
			refcache_refobj_dec(&__test_obj);
		}
		if(i ==10+TEST_ROUND*2 && id==1){
			/* should die soon */
			refcache_refobj_dec(&__test_obj);
		}else if(i>10+TEST_ROUND*2){
			do_sleep(1);
		}
		i++;
		if(i%10==0)
			kprintf("XX cpu%d g %d\n", id, __test_obj.refcount);
		if(id % 4==0)
			do_sleep(id*2);
#else
		do_sleep(1);
#endif
	}
}

