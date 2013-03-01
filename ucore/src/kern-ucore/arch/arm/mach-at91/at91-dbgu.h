/*
 * =====================================================================================
 *
 *       Filename:  at91-dbgu.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/24/2012 12:39:30 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef __AT91_DBGU_H
#define __AT91_DBGU_H

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Debug Unit
// *****************************************************************************
// *** Register offset in AT91S_DBGU structure ***
#define DBGU_CR         ( 0)	// Control Register
#define DBGU_MR         ( 4)	// Mode Register
#define DBGU_IER        ( 8)	// Interrupt Enable Register
#define DBGU_IDR        (12)	// Interrupt Disable Register
#define DBGU_IMR        (16)	// Interrupt Mask Register
#define DBGU_CSR        (20)	// Channel Status Register
#define DBGU_RHR        (24)	// Receiver Holding Register
#define DBGU_THR        (28)	// Transmitter Holding Register
#define DBGU_BRGR       (32)	// Baud Rate Generator Register
#define DBGU_CIDR       (64)	// Chip ID Register
#define DBGU_EXID       (68)	// Chip ID Extension Register
#define DBGU_FNTR       (72)	// Force NTRST Register
#define DBGU_RPR        (256)	// Receive Pointer Register
#define DBGU_RCR        (260)	// Receive Counter Register
#define DBGU_TPR        (264)	// Transmit Pointer Register
#define DBGU_TCR        (268)	// Transmit Counter Register
#define DBGU_RNPR       (272)	// Receive Next Pointer Register
#define DBGU_RNCR       (276)	// Receive Next Counter Register
#define DBGU_TNPR       (280)	// Transmit Next Pointer Register
#define DBGU_TNCR       (284)	// Transmit Next Counter Register
#define DBGU_PTCR       (288)	// PDC Transfer Control Register
#define DBGU_PTSR       (292)	// PDC Transfer Status Register
// -------- DBGU_CR : (DBGU Offset: 0x0) Debug Unit Control Register -------- 
#define AT91C_US_RSTRX            (0x1 <<  2)	// (DBGU) Reset Receiver
#define AT91C_US_RSTTX            (0x1 <<  3)	// (DBGU) Reset Transmitter
#define AT91C_US_RXEN             (0x1 <<  4)	// (DBGU) Receiver Enable
#define AT91C_US_RXDIS            (0x1 <<  5)	// (DBGU) Receiver Disable
#define AT91C_US_TXEN             (0x1 <<  6)	// (DBGU) Transmitter Enable
#define AT91C_US_TXDIS            (0x1 <<  7)	// (DBGU) Transmitter Disable
#define AT91C_US_RSTSTA           (0x1 <<  8)	// (DBGU) Reset Status Bits
// -------- DBGU_MR : (DBGU Offset: 0x4) Debug Unit Mode Register -------- 
#define AT91C_US_PAR              (0x7 <<  9)	// (DBGU) Parity type
#define 	AT91C_US_PAR_EVEN                 (0x0 <<  9)	// (DBGU) Even Parity
#define 	AT91C_US_PAR_ODD                  (0x1 <<  9)	// (DBGU) Odd Parity
#define 	AT91C_US_PAR_SPACE                (0x2 <<  9)	// (DBGU) Parity forced to 0 (Space)
#define 	AT91C_US_PAR_MARK                 (0x3 <<  9)	// (DBGU) Parity forced to 1 (Mark)
#define 	AT91C_US_PAR_NONE                 (0x4 <<  9)	// (DBGU) No Parity
#define 	AT91C_US_PAR_MULTI_DROP           (0x6 <<  9)	// (DBGU) Multi-drop mode
#define AT91C_US_CHMODE           (0x3 << 14)	// (DBGU) Channel Mode
#define 	AT91C_US_CHMODE_NORMAL               (0x0 << 14)	// (DBGU) Normal Mode: The USART channel operates as an RX/TX USART.
#define 	AT91C_US_CHMODE_AUTO                 (0x1 << 14)	// (DBGU) Automatic Echo: Receiver Data Input is connected to the TXD pin.
#define 	AT91C_US_CHMODE_LOCAL                (0x2 << 14)	// (DBGU) Local Loopback: Transmitter Output Signal is connected to Receiver Input Signal.
#define 	AT91C_US_CHMODE_REMOTE               (0x3 << 14)	// (DBGU) Remote Loopback: RXD pin is internally connected to TXD pin.
// -------- DBGU_IER : (DBGU Offset: 0x8) Debug Unit Interrupt Enable Register -------- 
#define AT91C_US_RXRDY            (0x1 <<  0)	// (DBGU) RXRDY Interrupt
#define AT91C_US_TXRDY            (0x1 <<  1)	// (DBGU) TXRDY Interrupt
#define AT91C_US_ENDRX            (0x1 <<  3)	// (DBGU) End of Receive Transfer Interrupt
#define AT91C_US_ENDTX            (0x1 <<  4)	// (DBGU) End of Transmit Interrupt
#define AT91C_US_OVRE             (0x1 <<  5)	// (DBGU) Overrun Interrupt
#define AT91C_US_FRAME            (0x1 <<  6)	// (DBGU) Framing Error Interrupt
#define AT91C_US_PARE             (0x1 <<  7)	// (DBGU) Parity Error Interrupt
#define AT91C_US_TXEMPTY          (0x1 <<  9)	// (DBGU) TXEMPTY Interrupt
#define AT91C_US_TXBUFE           (0x1 << 11)	// (DBGU) TXBUFE Interrupt
#define AT91C_US_RXBUFF           (0x1 << 12)	// (DBGU) RXBUFF Interrupt
#define AT91C_US_COMM_TX          (0x1 << 30)	// (DBGU) COMM_TX Interrupt
#define AT91C_US_COMM_RX          (0x1 << 31)	// (DBGU) COMM_RX Interrupt
// -------- DBGU_IDR : (DBGU Offset: 0xc) Debug Unit Interrupt Disable Register -------- 
// -------- DBGU_IMR : (DBGU Offset: 0x10) Debug Unit Interrupt Mask Register -------- 
// -------- DBGU_CSR : (DBGU Offset: 0x14) Debug Unit Channel Status Register -------- 
// -------- DBGU_FNTR : (DBGU Offset: 0x48) Debug Unit FORCE_NTRST Register -------- 
#define AT91C_US_FORCE_NTRST      (0x1 <<  0)	// (DBGU) Force NTRST in JTAG

#endif
