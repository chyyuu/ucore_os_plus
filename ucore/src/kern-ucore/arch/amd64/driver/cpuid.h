/*
 * =====================================================================================
 *
 *       Filename:  cpuid.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/20/2013 04:48:56 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef __ARCH_CPUID_H
#define __ARCH_CPUID_H

typedef enum{
	CPUID_FEATURE_X2APIC,
	CPUID_FEATURE_APIC,
	CPUID_FEATURE_PAGE1G,
}CPUID_INFO_TYPE;


int cpuid_check_feature(CPUID_INFO_TYPE type);
const char* cpuid_vendor_string();

#endif

