/*
 * =====================================================================================
 *
 *       Filename:  yaffs_osglue.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/08/2012 03:28:43 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <types.h>
#include <slab.h>
#include <sem.h>

#include "yaffs_trace.h"

extern volatile size_t ticks;

//unsigned yaffs_trace_mask = 0xFFFFFFFF;
unsigned yaffs_trace_mask =
//	YAFFS_TRACE_SCAN |
//	YAFFS_TRACE_GC |
//	YAFFS_TRACE_ERASE |
	YAFFS_TRACE_ERROR |
//	YAFFS_TRACE_TRACING |
//	YAFFS_TRACE_ALLOCATE |
	YAFFS_TRACE_BAD_BLOCKS |
//	YAFFS_TRACE_VERIFY |
  //YAFFS_TRACE_OS|
	0;
int yaffs_errno;
static semaphore_t yaffs_sem;

uint32_t yaffsfs_CurrentTime(void)
{
  return (uint32_t)ticks;
}

void yaffsfs_Lock()
{
  down(&yaffs_sem);
}

void yaffsfs_Unlock()
{
  up(&yaffs_sem);
}

void yaffsfs_SetError(int err)
{
  yaffs_errno = err;
}

void *yaffsfs_malloc(size_t size)
{
  return kmalloc(size);
}

void yaffsfs_free(void *ptr)
{
  kfree(ptr);
}

void yaffsfs_OSInitialisation()
{
  sem_init(&yaffs_sem, 1);
}

int yaffsfs_GetLastError()
{
  return yaffs_errno;
}

