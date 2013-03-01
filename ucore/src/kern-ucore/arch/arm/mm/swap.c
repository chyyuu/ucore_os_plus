/*
 * =====================================================================================
 *
 *       Filename:  swap.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/22/2012 07:32:06 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <swap.h>
#include <pmm.h>
#include <mmu.h>
#include <vmm.h>
#include <memlayout.h>
#include <swapfs.h>
#include <assert.h>
#include <stdio.h>
#include <atomic.h>
#include <sync.h>
#include <slab.h>
#include <string.h>
#include <error.h>
#include <kio.h>

void check_mm_shm_swap(void)
{
	kprintf("check_mm_shm_swap NOT impl\n");
}

void check_mm_swap(void)
{
	kprintf("check_mm_swap NOT impl\n");
}
