/*
 * =====================================================================================
 *
 *       Filename:  lockfree_stack.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/30/2013 06:40:02 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef __LIBS_LOCKFREE_STACK_H
#define __LIBS_LOCKFREE_STACK_H

#include <atomic.h>

struct lockfree_stack_entry{
	volatile struct lockfree_stack_entry *link;
};

struct lockfree_stack{
	volatile struct lockfree_stack_entry *top;
	/* avoid ABA problem */
	unsigned long volatile oc;
	atomic_t count;
};

static inline void lockfree_stack_init(struct lockfree_stack *lfs)
{
	lfs->top = NULL;
	atomic_set(&lfs->count, 0);
	lfs->oc = 0;
}

static inline void lockfree_stack_push(struct lockfree_stack *lfs, 
		struct lockfree_stack_entry* entry)
{
	volatile struct lockfree_stack_entry *oldtop;
	do{
		oldtop = lfs->top;
		entry->link = oldtop;
		
	}while(!CAS(&lfs->top, oldtop, entry));
	atomic_inc(&lfs->count);
}

static inline struct lockfree_stack_entry* lockfree_stack_pop(struct lockfree_stack *lfs)
{
	volatile unsigned long oc;
	struct lockfree_stack_entry *oldtop;
	do{
		oc = lfs->oc;
		oldtop = (struct lockfree_stack_entry*)lfs->top;
		if(!oldtop) return NULL;
	}while(!CAS2(&lfs->top, oldtop, oc, (void*)oldtop->link, oc+1));
	atomic_dec(&lfs->count);
	return oldtop;
}

#endif
