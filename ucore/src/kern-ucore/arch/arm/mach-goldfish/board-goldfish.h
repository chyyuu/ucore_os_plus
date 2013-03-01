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

#ifndef  MACH_BOARD_GOLDFISH_H
#define  MACH_BOARD_GOLDFISH_H

#define GOLDFISH_IO_START 0xFF000000
#define GOLDFISH_IO_FREE_START 0xff010000
#define GOLDFISH_UART0 (GOLDFISH_IO_FREE_START+0x2000)
#define GOLDFISH_PDEV_BUS (GOLDFISH_IO_START+0x1000)

#define GOLDFISH_TIMER0_1_BASE (GOLDFISH_IO_START+0x3000)

#define GOLDFISH_VIC_BASE      (GOLDFISH_IO_START+0x00)

#ifndef __io_address
#define __io_address(x) (x)
#endif

//IRQ 
#define TIMER0_IRQ 3

#define UART_RXIM (1<<4)

//extern macro

#define SDRAM0_START UCONFIG_DRAM_START
#define SDRAM0_SIZE  UCONFIG_DRAM_SIZE	//256M

#define IO_SPACE_START GOLDFISH_IO_START
#define IO_SPACE_SIZE  0x1000000

//#define HAS_RAMDISK
//#define HAS_FRAMEBUFFER
//#define HAS_SHARED_KERNMEM
//#define KERNEL_PHY_BASE 0x100000

#ifndef __ASSEMBLY__

#define UART0_TX 		((volatile unsigned char*) GOLDFISH_UART0 + 0x00)
//#define INITIAL_LOAD    ((volatile uintptr_t *) (0x1000))

extern void board_init(void);

#endif

#endif
