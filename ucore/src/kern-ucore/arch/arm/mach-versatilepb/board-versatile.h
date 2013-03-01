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

#ifndef  MACH_BOARD_VERSATILE_H
#define  MACH_BOARD_VERSATILE_H

#define VERSATILEPB_IO_START 0x10000000
#define VERSATILEPB_UART0 0x101f1000

#define PL011_UARTRSR 0x004
#define PL011_UARTFR 0x018
/* Qemu buggy: RXFF does not work, use RXFE */
#define PL011_RXFF    (1<<6)
#define PL011_RXFE    (1<<4)

#define VERSATILE_TIMER0_1_BASE (0x101e2000)
#define VERSATILE_TIMER2_1_BASE (0x101e3000)

#define VERSATILE_VIC_BASE      (0x10140000)

#define __io_address(x) (x)

//IRQ 
#define TIMER0_IRQ 4
#define UART_IRQ   12

#define  UARTIMSC 0x038
#define   UARTMI 0x040
#define  UARTICR 0x044

#define UART_RXIM (1<<4)

//extern macro

#define SDRAM0_START 0x0000
#define SDRAM0_SIZE  0x4000000	//64M

#define IO_SPACE_START VERSATILEPB_IO_START
#define IO_SPACE_SIZE  0x200000

#define HAS_RAMDISK
#define HAS_SIM_YAFFS
#define HAS_SHARED_KERNMEM
//#define KERNEL_PHY_BASE 0x100000

#ifndef __ASSEMBLY__

#define UART0_TX 		((volatile unsigned char*) VERSATILEPB_UART0)
//#define INITIAL_LOAD    ((volatile uintptr_t *) (0x1000))

extern void board_init(void);

#endif

#endif
