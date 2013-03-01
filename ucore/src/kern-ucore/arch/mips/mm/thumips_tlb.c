/*
 * =====================================================================================
 *
 *       Filename:  thumips_tlb.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/06/2012 10:23:50 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#include <defs.h>
#include <arch.h>
#include <stdio.h>
#include <string.h>
#include <mmu.h>
#include <memlayout.h>
#include <pmm.h>
#include <thumips_tlb.h>

// invalidate both TLB 
// (clean and flush, meaning we write the data back)
void tlb_invalidate(pde_t * pgdir, uintptr_t la)
{
	tlb_invalidate_all();
}

void tlb_invalidate_all()
{				//kprintf("\n\n\n\ntlb_invalidate_all()\n\n\n\n");
	int i;
	for (i = 0; i < 128 * 128; i++)
		write_one_tlb(i, 0, 0x80000000 + (i << 20), 0, 0);
}
