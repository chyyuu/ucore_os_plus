#include <types.h>
#include <kio.h>
#include <mmu.h>
#include <string.h>
#include <pmm.h>

static void domainAccessSet(uint32_t value, uint32_t mask);
static void controlSet(uint32_t value, uint32_t mask);

/* Page Tables */
/* VADDRESS, PTADDRESS, MASTERPTADDRESS, PTTYPE, DOM */
//static Pagetable masterPT = {0x30700000, 0x30700000, 0x30700000, MASTER, 0};
/* Regions */
/* vAddress, pageSize, numPages, AP, CB, pAddress, PT */
// note: for now, no cached, otherwise the interrupt doesn't finish
//static Region masterRegion = {0x00000000, SECTIONPAGE, 4096, PTE_RWNA, PTE_cb, 0x00000000, &masterPT};

/* mmuInitPT
 * Initialize a page table by filling it with fault PTE */
int mmu_init_pdt(Pagetable * pt)
{
	uint32_t PTE, *PTEptr;
	PTEptr = (uint32_t *) KADDR(pt->ptAddress);

	//kprintf("Pagetable type %d\n", pt->type);
	switch (pt->type) {
	case COARSE:
		memset(PTEptr, 0, NPTEENTRY * 4);
		return 0;
	case MASTER:
		memset(PTEptr, 0, NPDEENTRY * 4);
		return 0;
	default:
		kprintf("Pagetage type incorrect: %d\n", pt->type);
		return -1;
	}
/*  	
	asm volatile (
		"mov r0, %0;"
		"mov r1, %0;"
		::"r" (PTE):"r0","r1");
	
	for (; index > 0; index--)
	{
		asm volatile (
			"STMIA %0!, {r0-r1};" 
			"STMIA %0!, {r0-r1};"
			"STMIA %0!, {r0-r1};"
			"STMIA %0!, {r0-r1};"
			"STMIA %0!, {r0-r1};"
			"STMIA %0!, {r0-r1};"
			"STMIA %0!, {r0-r1};"
			"STMIA %0!, {r0-r1};"
			:"=r" (PTEptr):"r" (PTEptr):"r0","r1");
		
	}
	//kprintf("Pagetable written to 0x%08xd\n", PTEptr);
	//kprintf("Pagetable configured\n");
*/
	return -1;
}

/* 16 domains */
static void domainAccessSet(uint32_t value, uint32_t mask)
{
	uint32_t c3format;
	asm volatile ("MRC p15, 0, %0, c3, c0, 0"	/* read domain register */
		      :"=r" (c3format)
	    );
	c3format &= ~mask;	/* clear bits that change */
	c3format |= value;	/* set bits that change */
	asm volatile ("MCR p15, 0, %0, c3, c0, 0"	/* write domain register */
		      ::"r" (c3format)
	    );
}

/* controlSet
 * sets the control bits register in CP15:c1 */
static void controlSet(uint32_t value, uint32_t mask)
{
	uint32_t c1format;
	asm volatile ("MRC p15, 0, %0, c1, c0, 0"	/* read control register */
		      :"=r" (c1format)
	    );
	c1format &= ~mask;	/* clear bits that change */
	c1format |= value;	/* set bits that change */
	asm volatile ("MCR p15, 0, %0, c1, c0, 0"	/* write control register */
		      ::"r" (c1format)
	    );
}

/* mmu_init - initialize the virtual memory management */
// Supposed already attached and initialized
/* 1. Initialize the page tables in main memory by filling them with FAULT entries.
 * 2. Fill in the page tables with translations that map regions to physical memory.
 * 3. Activate the page tables.
 * 4. Assign domain access rights.
 * 5. Enable the memory management unit and cache hardware.
 * */
void mmu_init(void)
{
	uint32_t enable, change;

	/* Part 4 Set Domain Access */
	domainAccessSet(DOM3CLT, CHANGEALLDOM);	/* set Domain Access */

	/* Part 5 Enable MMU, caches and write buffer */
	enable = ENABLEMMU | ENABLEICACHE | ENABLEDCACHE | ENABLEHIGHEVT;
	change = CHANGEMMU | CHANGEICACHE | CHANGEDCACHE | CHANGEHIGHEVT;
#ifdef __MACH_ARM_ARMV6
	enable |= ENABLENEWPT;
	change |= CHANGENEWPT;
#endif

	/* enable cache and MMU */
	controlSet(enable, change);
}
