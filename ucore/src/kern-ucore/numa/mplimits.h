/*
 * =====================================================================================
 *
 *       Filename:  mplimits.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/27/2013 10:54:21 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#ifndef __NUMA_MPLIMITS_H
#define __NUMA_MPLIMITS_H

#include <arch.h>

#define NCPU		UCONFIG_NR_CPUS
#define MAX_NUMA_NODES	UCONFIG_NR_NUMA_NODES
#define MAX_NUMA_MEMS	UCONFIG_NR_MEMS_PER_NODE
#define MAX_NUMA_MEM_ZONES 16

#ifndef CACHELINE
#warning CACHELINE not defined
#define CACHELINE 64
#endif

#endif

