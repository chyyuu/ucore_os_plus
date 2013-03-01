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

#ifndef  MACH_BOARD_AT91_H
#define  MACH_BOARD_AT91_H

#define AT91SAM_IO_START 0xF0000000
#define AT91SAM_DBGU_BASE 0xFFFFEE00
#define AT91C_BASE_ECC            (0xFFFFE200)	// (ECC) Base Address
#define AT91C_BASE_SMC            (0xFFFFE800)	// (SMC) Base Address
#define AT91C_BASE_MATRIX         (0xFFFFEA00)	// (MATRIX) Base Address
#define AT91C_BASE_PMC            (0xFFFFFC00)	// (PMC) Base Address

#define AT91C_BASE_AIC            (0xFFFFF000)	// (AIC) Base Address
#define AT91C_BASE_PIOA           (0xFFFFF200)	// (PIOA) Base Address
#define AT91C_BASE_PIOB           (0xFFFFF400)	// (PIOB) Base Address
#define AT91C_BASE_PIOC           (0xFFFFF600)	// (PIOC) Base Address
#define AT91C_BASE_PIOD           (0xFFFFF800)	// (PIOD) Base Address

#define AT91C_BASE_PIT           (0xFFFFFD30)	// (PIT) Base Address

#define AT91C_BASE_EBI2          (0x30000000)	//EBI Chip Select 2

#define AT91C_SMARTMEDIA_BASE	0x40000000

// *****************************************************************************
//               PERIPHERAL ID DEFINITIONS FOR AT91SAM9264
// *****************************************************************************
#define AT91C_ID_FIQ              ( 0)	// Advanced Interrupt Controller (FIQ)
#define AT91C_ID_SYS              ( 1)	// System Controller
#define AT91C_ID_PIOA             ( 2)	// Parallel IO Controller A
#define AT91C_ID_PIOB             ( 3)	// Parallel IO Controller B
#define AT91C_ID_PIOC             ( 4)	// Parallel IO Controller C
#define AT91C_ID_PIOD_E           ( 5)	// Parallel IO Controller D and E
#define AT91C_ID_TRNG             ( 6)	//
#define AT91C_ID_US0              ( 7)	// USART 0
#define AT91C_ID_US1              ( 8)	// USART 1
#define AT91C_ID_US2              ( 9)	// USART 2
#define AT91C_ID_US3              (10)	// USART 2
#define AT91C_ID_MCI0             (11)	// Multimedia Card Interface 0
#define AT91C_ID_TWI0             (12)	// TWI 0
#define AT91C_ID_TWI1             (13)	// TWI 1
#define AT91C_ID_SPI0             (14)	// Serial Peripheral Interface
#define AT91C_ID_SPI1             (15)	// Serial Peripheral Interface
#define AT91C_ID_SSC0             (16)	// Serial Synchronous Controller 0
#define AT91C_ID_SSC1             (17)	// Serial Synchronous Controller 1
#define AT91C_ID_TC               (18)	// Timer Counter 0, 1, 2, 3, 4, 5
#define AT91C_ID_PWMC             (19)	// Pulse Width Modulation Controller
#define AT91C_ID_TSADC            (20)	// Touch Screen Controller
#define AT91C_ID_HDMA             (21)	// HDMA
#define AT91C_ID_UHPHS            (22)	// USB Host High Speed
#define AT91C_ID_LCDC             (23)	// LCD Controller
#define AT91C_ID_AC97C            (24)	// AC97 Controller
#define AT91C_ID_EMAC             (25)	// Ethernet MAC
#define AT91C_ID_ISI              (26)	// Image Sensor Interface
#define AT91C_ID_UDPHS            (27)	// USB Device HS
#define AT91C_ID_TDES             (28)	// TDES/SHA/AES/TRNG
#define AT91C_ID_MCI1             (29)	// Multimedia Card Interface 1
#define AT91C_ID_VDEC             (30)	// Video Decoder
#define AT91C_ID_IRQ0             (31)	// Advanced Interrupt Controller (IRQ0)
#define AT91C_ALL_INT             (0xFFFFFFFF)	// ALL VALID INTERRUPTS

#include "at91-aic.h"
#include "at91-uart.h"
#include "at91-dbgu.h"

/* PIT definition */
#define PIT_MR                  (0)	//PIT  Mode Register
#define PIT_SR                  (4)	//PIT Status Register
#define PIT_PIVR                (8)	//PIT Periodic Interval Value Register
#define PIT_PIIR                (12)	//PIT Periodic Interval Image Register

//extern macro
//
//CONFIG

#define SDRAM0_START 0x70000000
#define SDRAM0_SIZE  (0x4000000)	//64M

#define IO_SPACE_START AT91SAM_IO_START
#define IO_SPACE_SIZE  0x10000000

#define HAS_RAMDISK
#define HAS_NANDFLASH
#define HAS_SDS
#define HAS_SHARED_KERNMEM

//#define KERNEL_PHY_BASE 0x70100000

#ifndef __ASSEMBLY__

#define UART0_TX 		((volatile unsigned char*) (AT91SAM_DBGU_BASE+DBGU_THR))

void board_init(void);
//used by bootloader, MUST be identical to linker script
//#define INITIAL_LOAD    ((volatile uintptr_t *) (0x73f01000))

#endif

#endif
