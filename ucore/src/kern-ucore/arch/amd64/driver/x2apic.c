/*
 * =====================================================================================
 *
 *       Filename:  x2apic.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/20/2013 05:41:27 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <kio.h>
#include <arch.h>
#include <cpuid.h>
#include <lapic.h>
#include <assert.h>

static void x2_cpu_init(struct lapic_chip* _this)
{
	panic("x2_cpu_init not implemented");
}

static struct lapic_chip x2apic_chip = {
	.cpu_init = x2_cpu_init,
};

struct lapic_chip* x2apic_lapic_init_early(void){
	if(!cpuid_check_feature(CPUID_FEATURE_X2APIC))
		return NULL;
	uint64_t apic_bar = readmsr(MSR_APIC_BAR);
	if (!(apic_bar & APIC_BAR_X2APIC_EN) || !(apic_bar & APIC_BAR_XAPIC_EN))
		return NULL;
	kprintf("x2apic_lapic_init: Using x2apic\n");

	return &x2apic_chip;
}

