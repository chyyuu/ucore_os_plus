/* ----------------------------------------------------------------------------
 *         ATMEL Microcontroller Software Support  -  ROUSSET  -
 * ----------------------------------------------------------------------------
 * Copyright (c) 2006, Atmel Corporation

 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ----------------------------------------------------------------------------
 * File Name           : nand_ids.h
 * Object              :
 * Creation            : NLe Sep 28th 2006
 *-----------------------------------------------------------------------------
 */
#ifndef _NAND_IDS_H
#define _NAND_IDS_H

#include "nandflash.h"

/* Supported NandFlash devices */
const static struct SNandInitInfo NandFlash_InitInfo[] = {
	{0x2cca, 0x800, 0x20000, 0x800, 0x40, 0x1,
	 "Micron MT29F2G16AAB 256MB\0"},
	{0x2cda, 0x800, 0x20000, 0x800, 0x40, 0x0,
	 "Micron MT29F2G08AAC 256MB\0"},
	{0x2caa, 0x800, 0x20000, 0x800, 0x40, 0x0,
	 "Micron MT29F2G08ABD 256MB\0"},
	{0xecda, 0x800, 0x20000, 0x800, 0x40, 0x0,
	 "Samsung K9F2G08U0M 256MB\0"},
	{0xecaa, 0x800, 0x20000, 0x800, 0x40, 0x0,
	 "Samsung K9F2G08U0A 256MB\0"},
	{0x20aa, 0x800, 0x20000, 0x800, 0x40, 0x0,
	 "ST Micro NAND02GR3B 256MB\0"},
	{0,}
};

#endif
