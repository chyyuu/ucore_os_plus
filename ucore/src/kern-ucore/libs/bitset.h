/*
 * Copyright (C) 2010 by Joseph A. Marrero and Shrewd LLC. http://www.manvscode.com/
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef _LIBS_BITSET_H_
#define _LIBS_BITSET_H_

#include <types.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t byte;
typedef struct bitset {
	byte*  array;
	size_t bit_size;
} bitset_t;

#if CHAR_BIT == 8
#define bits_to_bytes(bits)                (((bits) + 7) >> 3)
#define bit_to_index(bit)                  ((bit) >> 3)
#else
#define bits_to_bytes(bits)                (((bits) + CHAR_BIT - 1) / CHAR_BIT)
#define bit_to_index(bit)                  ((bit) / CHAR_BIT )
#endif


uint8_t        bitset_create  ( bitset_t *p_bitset, size_t bits );
void           bitset_destroy ( bitset_t *p_bitset );
void           bitset_clear   ( bitset_t *p_bitset );
uint8_t        bitset_resize  ( bitset_t *p_bitset, size_t bits );
char*          bitset_string  ( bitset_t *p_bitset );
#define        bitset_bits(p_bitset)              ((p_bitset)->bit_size)

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L)
void    bitset_set     ( bitset_t *p_bitset, size_t bit );
void    bitset_unset   ( bitset_t *p_bitset, size_t bit );
uint8_t bitset_test    ( const bitset_t *p_bitset, size_t bit );
#else
static inline void bitset_set( bitset_t *p_bitset, size_t bit )
{
	assert( p_bitset );
	assert( bit < p_bitset->bit_size );
	p_bitset->array[ bit_to_index(bit) ] |= (0x01 << (bit % CHAR_BIT));
}

static inline void bitset_unset( bitset_t *p_bitset, size_t bit )
{
	assert( p_bitset );
	assert( bit < p_bitset->bit_size );
	p_bitset->array[ bit_to_index(bit) ] &= ~(0x01 << (bit % CHAR_BIT));
}

static inline uint8_t bitset_test( const bitset_t *p_bitset, size_t bit )
{
	assert( p_bitset );
	return p_bitset->array[ bit_to_index(bit) ] & (0x01 << (bit % CHAR_BIT));
}
#endif

#ifdef __cplusplus
} /* external C linkage */
#endif
#endif /* _BITSET_H_ */
