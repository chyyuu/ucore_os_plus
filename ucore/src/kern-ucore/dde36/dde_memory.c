/*
 * =====================================================================================
 *
 *       Filename:  dde_memory.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/30/2012 11:30:38 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <slab.h>

#include <dde_kit/memory.h>

void *dde_kit_simple_malloc(dde_kit_size_t size)
{
  return kmalloc(size);
}

void dde_kit_simple_free(void *p)
{
  if(!p)
    return;
  kfree(p);
}
