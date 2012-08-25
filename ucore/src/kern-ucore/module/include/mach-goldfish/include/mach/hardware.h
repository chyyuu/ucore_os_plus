/* include/asm-arm/arch-goldfish/hardware.h
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

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <asm/sizes.h>

/*
 * Where in virtual memory the IO devices (timers, system controllers
 * and so on)
 */
#define IO_BASE			0xfe000000                 // VA of IO 
#define IO_SIZE			0x00800000                 // How much?
#define IO_START		0xff000000                 // PA of IO

#define GOLDFISH_INTERRUPT_BASE     (0x0)
#define GOLDFISH_INTERRUPT_STATUS       (0x00) // number of pending interrupts
#define GOLDFISH_INTERRUPT_NUMBER       (0x04)
#define GOLDFISH_INTERRUPT_DISABLE_ALL  (0x08)
#define GOLDFISH_INTERRUPT_DISABLE      (0x0c)
#define GOLDFISH_INTERRUPT_ENABLE       (0x10)

#define GOLDFISH_PDEV_BUS_BASE      (0x1000)
#define GOLDFISH_PDEV_BUS_END       (0x100)

#define GOLDFISH_TTY_BASE       (0x2000)
#define GOLDFISH_TIMER_BASE     (0x3000)

/* macro to get at IO space when running virtually */
#define IO_ADDRESS(x) ((x) + IO_BASE)

#endif
