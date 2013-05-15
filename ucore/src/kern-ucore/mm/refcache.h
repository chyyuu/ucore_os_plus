/*
 * =====================================================================================
 *
 *       Filename:  refcache.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/15/2013 09:47:10 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#ifndef __MM_REFCACHE_H
#define __MM_REFCACHE_H

#include <arch.h>
#include <mp.h>
#include <list.h>
#include <memlayout.h>
#include <atomic.h>
#include <spinlock.h>
#include <percpu.h>

#define REF_CACHE_SLOT_SHIFT 6
#define REF_CACHE_SLOT (1<<6)
#define REFOBJ_FLAG_DIRTY 1

/* this struct should be embedded into 
 * a reference-counted object
 */
struct refobj{
	/* global */
	unsigned int refcount;
	int review_epoch;
	unsigned int flag;
	list_entry_t review_link;
	spinlock_s lock;
	void (*onzero)(struct refobj*);
};

#define set_refobj_callback(obj, func) \
	do{(obj)->onzero = func;}while(0)

struct refway{
	int delta;
	struct refobj *obj;
};

/* per core cache */
struct refcache
{
	struct refway way[REF_CACHE_SLOT];
	int local_epoch;
	list_entry_t review_head;
};

DECLARE_PERCPU(struct refcache, refcaches);

void refcache_init(void);
void refcache_refobj_init(struct refobj *obj);
void refcache_refobj_inc(struct refobj *obj);
void refcache_refobj_dec(struct refobj *obj);
void refcache_tick(void);

int krefcache_cleaner(void *arg) __attribute__ ((noreturn));
#endif
