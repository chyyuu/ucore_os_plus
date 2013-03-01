/*
 * =====================================================================================
 *
 *       Filename:  kmalloc_glue.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/22/2012 04:02:49 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <linux/kernel.h>
#include <linux/gfp.h>
#include <linux/err.h>
#include <linux/init.h>

#define __NO_UCORE_TYPE__
#include <module.h>
#include <kio.h>
#include <slab.h>

void *__kmalloc(size_t size, gfp_t flags)
{
	//kprintf("__kmalloc %d %08x\n", size, flags);
	void *ptr = kmalloc(size);
	if (flags | __GFP_ZERO) {
		memset(ptr, 0, size);
	}
	return ptr;
}

void *krealloc(const void *p, size_t new_size, gfp_t flags)
{
	void *ret;

	if (!new_size) {
		kfree(p);
		return (void *)16;
	}

	ret = __kmalloc(new_size, flags);
	if (ret && p != ret)
		kfree(p);

	return ret;
}
