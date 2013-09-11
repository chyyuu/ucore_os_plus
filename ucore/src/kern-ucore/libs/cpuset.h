/*
 * =====================================================================================
 *
 *       Filename:  cpuset.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/27/2013 10:42:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#ifndef _LIBS_CPUSET_H
#define _LIBS_CPUSET_H

#include <types.h>
#include <mplimits.h>
#include <assert.h>

#define __cpuset_bits_to_bytes(bits)                (((bits) + 7) >> 3)
#define __cpuset_bit_to_index(bit)                  ((bit) >> 3)
struct __cpuset{
	uint8_t map[(NCPU+7)/8];
};

typedef struct __cpuset cpuset_t;

static inline void cpuset_set(cpuset_t *s, size_t bit)
{
	assert(s && (bit<NCPU));
	s->map[ __cpuset_bit_to_index(bit) ] |= (0x01 << (bit & 0x7));
}

static inline void cpuset_unset(cpuset_t *s, size_t bit)
{
	assert(s && (bit<NCPU));
	s->map[ __cpuset_bit_to_index(bit) ] &= ~(0x01 << (bit & 0x7));
}

static inline int cpuset_test(const cpuset_t *s, size_t bit)
{
	assert(s && (bit<NCPU));
	return s->map[ __cpuset_bit_to_index(bit) ] & (0x01 << (bit & 0x7));
}

#endif

