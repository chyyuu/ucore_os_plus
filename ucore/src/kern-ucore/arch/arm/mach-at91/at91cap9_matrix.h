/*
 * arch/arm/mach-at91/include/mach/at91cap9_matrix.h
 *
 *  Copyright (C) 2007 Stelian Pop <stelian.pop@leadtechdesign.com>
 *  Copyright (C) 2007 Lead Tech Design <www.leadtechdesign.com>
 *  Copyright (C) 2006 Atmel Corporation.
 *
 * Memory Controllers (MATRIX, EBI) - System peripherals registers.
 * Based on AT91CAP9 datasheet revision B (Preliminary).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef AT91CAP9_MATRIX_H
#define AT91CAP9_MATRIX_H

#define AT91_MATRIX_EBICSA	( 0x128)	/* EBI Chip Select Assignment Register */
#define		AT91_MATRIX_EBI_CS1A		(1 << 1)	/* Chip Select 1 Assignment */
#define			AT91_MATRIX_EBI_CS1A_SMC		(0 << 1)
#define			AT91_MATRIX_EBI_CS1A_BCRAMC		(1 << 1)
#define		AT91_MATRIX_EBI_CS3A		(1 << 3)	/* Chip Select 3 Assignment */
#define			AT91_MATRIX_EBI_CS3A_SMC		(0 << 3)
#define			AT91_MATRIX_EBI_CS3A_SMC_SMARTMEDIA	(1 << 3)
#define		AT91_MATRIX_EBI_CS4A		(1 << 4)	/* Chip Select 4 Assignment */
#define			AT91_MATRIX_EBI_CS4A_SMC		(0 << 4)
#define			AT91_MATRIX_EBI_CS4A_SMC_CF1		(1 << 4)
#define		AT91_MATRIX_EBI_CS5A		(1 << 5)	/* Chip Select 5 Assignment */
#define			AT91_MATRIX_EBI_CS5A_SMC		(0 << 5)
#define			AT91_MATRIX_EBI_CS5A_SMC_CF2		(1 << 5)
#define		AT91_MATRIX_EBI_DBPUC		(1 << 8)	/* Data Bus Pull-up Configuration */
#define		AT91_MATRIX_EBI_DQSPDC		(1 << 9)	/* Data Qualifier Strobe Pull-Down Configuration */
#define		AT91_MATRIX_EBI_VDDIOMSEL	(1 << 16)	/* Memory voltage selection */
#define			AT91_MATRIX_EBI_VDDIOMSEL_1_8V		(0 << 16)
#define			AT91_MATRIX_EBI_VDDIOMSEL_3_3V		(1 << 16)

#endif
