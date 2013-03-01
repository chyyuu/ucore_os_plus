/*
 * =====================================================================================
 *
 *       Filename:  board.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/17/2012 02:19:11 PM
 *       Revision:  none
 *       Compiler:  arm-linux-gcc 4.4
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef  BOOTLOADER_MACH_BOARD_H
#define  BOOTLOADER_MACH_BOARD_H

#ifdef PLATFORM_VERSATILEPB
#include "../../../kern-ucore/arch/arm/mach-versatilepb/board.h"
#elif PLATFORM_AT91
#include "../../../kern-ucore/arch/arm/mach-at91/board.h"
#elif PLATFORM_RASPBERRYPI
#include "../../../kern-ucore/arch/arm/mach-raspberrypi/board.h"
#elif PLATFORM_GOLDFISH
#include "../../../kern-ucore/arch/arm/mach-goldfish/board.h"
#else
#error Unknown platform
#endif

#endif
