/* include/asm-arm/arch-goldfish/memory.h
**
** Copyright (C) 2007 Google, Inc.
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
*/

#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

/*
 * Physical DRAM offset.
 */
#define PHYS_OFFSET	UL(0x00000000)
#define BUS_OFFSET	UL(0x00000000)

/*
 * Needed to allow framebuffer sizes > 1MB.
 * This must be a multiple of SZ_2M.
 *
 * Note that using 4MB allows us to use
 * framebuffers of up to 2 MB each (two are
 * used for normal operations).
 *
 * For some reason, using more than 4MB here
 * doesn't allow our emulated system to use
 * larger framebuffers.
 *
 */
#define CONSISTENT_DMA_SIZE      (2*SZ_2M)

/*
 * Virtual view <-> DMA view memory address translations
 * virt_to_bus: Used to translate the virtual address to an
 *              address suitable to be passed to set_dma_addr
 * bus_to_virt: Used to convert an address for DMA operations
 *              to an address that the kernel can use.
 */
#define __virt_to_bus(x)	(x - PAGE_OFFSET + BUS_OFFSET)
#define __bus_to_virt(x)	(x - BUS_OFFSET + PAGE_OFFSET)

#endif
