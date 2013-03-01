/* include/asm-arm/arch-goldfish/timer.h
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

#ifndef __ASM_ARCH_TIMER_H
#define __ASM_ARCH_TIMER_H

enum {
	TIMER_TIME_LOW = 0x00,	// get low bits of current time and update TIMER_TIME_HIGH
	TIMER_TIME_HIGH = 0x04,	// get high bits of time at last TIMER_TIME_LOW read
	TIMER_ALARM_LOW = 0x08,	// set low bits of alarm and activate it
	TIMER_ALARM_HIGH = 0x0c,	// set high bits of next alarm
	TIMER_CLEAR_INTERRUPT = 0x10,
	TIMER_CLEAR_ALARM = 0x14
};

#endif
