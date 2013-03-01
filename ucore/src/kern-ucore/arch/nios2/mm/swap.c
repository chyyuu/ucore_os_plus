#include <pmm.h>
#include <vmm.h>
#include <swap.h>
#include <swapfs.h>
#include <slab.h>
#include <assert.h>
#include <stdio.h>
#include <vmm.h>
#include <error.h>
#include <atomic.h>
#include <sync.h>
#include <string.h>
#include <stdlib.h>
#include <kio.h>

void check_mm_swap(void)
{
	kprintf("check_mm_swap() not supported!\n");
}

void check_mm_shm_swap(void)
{
	kprintf("check_mm_shm_swap() not supported!\n");
}
