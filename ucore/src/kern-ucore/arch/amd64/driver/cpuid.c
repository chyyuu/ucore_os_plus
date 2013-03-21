/*
 * =====================================================================================
 *
 *       Filename:  cpuid.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/20/2013 04:49:56 PM
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
#include <types.h>
#include "cpuid.h"

typedef enum {
	basic = 0,
	features = 1,
	cache_and_tlb = 2,
	serial_number = 3,
	cache_params = 4,           // Depends on ecx
	mwait = 5,
	thermal = 6,
	ext_features = 7,           // Depends on ecx
	dca = 9,
	perfmon = 0xa,
	topology = 0xb,             // Depends on ecx
	ext_state = 0xd,            // Depends on ecx
	qos = 0xf,                  // Depends on ecx

	extended_info = 0x80000000,
	extended_features = 0x80000001,
}cpuid_types;

struct leaf
{
	int valid;
	uint32_t a, b, c, d;
};
enum {
    MAX_BASIC = 0x10,
    MAX_EXTENDED = 0x9
};

static struct leaf basic_[MAX_BASIC], extended_[MAX_EXTENDED], features_, extended_features_;
static char vendor_[13];

static inline  struct leaf read_one(uint32_t eax, uint32_t ecx)
{
	uint32_t ebx, edx;
	__asm volatile("cpuid"
			: "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
			: "a" (eax), "c" (ecx));
	struct leaf l = {1, eax, ebx, ecx, edx};
	return l;

}

static void cpuid_readall()
{
	static int first_call_ = 1;
	int i;
	struct leaf l;
	if(!first_call_)
		return;
	first_call_ = 0;
	basic_[basic] = read_one(basic, 0);
	for(i=1;i<MAX_BASIC && i<=basic_[0].a;i++)
		basic_[i] = read_one(i, 0);
	extended_[0] = read_one(extended_info, 0);
	for(i=1;i<MAX_EXTENDED && i<= extended_[0].a - extended_info;i++)
		extended_[i] = read_one(extended_info + i, 0);
	l = read_one(basic, 0);
	uint32_t name[3] = {l.b, l.d, l.c};
	memcpy(vendor_, name, sizeof(name));
	vendor_[12] = 0;

	features_ = read_one(features, 0);
	extended_features_  = read_one(extended_features, 0);
}

int cpuid_check_feature(CPUID_INFO_TYPE type)
{
	cpuid_readall();
	switch(type){
		case CPUID_FEATURE_X2APIC:
			return features_.c & (1<<21);
		case CPUID_FEATURE_APIC:
			return features_.d & (1<<9);
		case CPUID_FEATURE_PAGE1G:
			return extended_features_.d & (1<<26);
		default:
			return 0;
	}
}

const char* cpuid_vendor_string()
{
	cpuid_readall();
	return vendor_;
}


