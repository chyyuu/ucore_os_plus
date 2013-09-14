/*
 * \brief   DDE Kit types
 * \author  Christian Helmuth
 * \date    2008-08-15
 *
 * DDE Kit defines a strict boundary between the host system, i.e., Genode, and
 * the emulated system, e.g., Linux. Therefore we cannot use Genode headers for
 * type definition directly. We must redefine all required types to prevent
 * potential incompatiblities and conflicts.
 */

/*
 * Copyright (C) 2008-2012 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

#ifndef _INCLUDE__DDE_KIT__TYPES_H_
#define _INCLUDE__DDE_KIT__TYPES_H_


enum {
	DDE_KIT_PAGE_SHIFT = 12,
	DDE_KIT_PAGE_SIZE  = 1 << DDE_KIT_PAGE_SHIFT,  /* our page size is 4096 */
};

typedef char dde_kit_int8_t;
typedef unsigned char dde_kit_uint8_t;
typedef short  dde_kit_int16_t;
typedef unsigned short dde_kit_uint16_t;
typedef int  dde_kit_int32_t;
typedef unsigned int dde_kit_uint32_t;
typedef long long  dde_kit_int64_t;
typedef unsigned long long dde_kit_uint64_t;

typedef unsigned long   dde_kit_addr_t;
typedef dde_kit_uint32_t   dde_kit_size_t;

#endif /* _INCLUDE__DDE_KIT__TYPES_H_ */
