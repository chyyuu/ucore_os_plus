/*
 * =====================================================================================
 *
 *       Filename:  at91-uart.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/24/2012 12:34:13 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef __AT91_UART_H
#define __AT91_UART_H

// *****************************************************************************
//              SOFTWARE API DEFINITION  FOR Usart
// *****************************************************************************
// *** Register offset in AT91S_USART structure ***
#define US_CR           ( 0)	// Control Register
#define US_MR           ( 4)	// Mode Register
#define US_IER          ( 8)	// Interrupt Enable Register
#define US_IDR          (12)	// Interrupt Disable Register
#define US_IMR          (16)	// Interrupt Mask Register
#define US_CSR          (20)	// Channel Status Register
#define US_RHR          (24)	// Receiver Holding Register
#define US_THR          (28)	// Transmitter Holding Register
#define US_BRGR         (32)	// Baud Rate Generator Register
#define US_RTOR         (36)	// Receiver Time-out Register
#define US_TTGR         (40)	// Transmitter Time-guard Register
#define US_FIDI         (64)	// FI_DI_Ratio Register
#define US_NER          (68)	// Nb Errors Register
#define US_IF           (76)	// IRDA_FILTER Register
#define US_RPR          (256)	// Receive Pointer Register
#define US_RCR          (260)	// Receive Counter Register
#define US_TPR          (264)	// Transmit Pointer Register
#define US_TCR          (268)	// Transmit Counter Register
#define US_RNPR         (272)	// Receive Next Pointer Register
#define US_RNCR         (276)	// Receive Next Counter Register
#define US_TNPR         (280)	// Transmit Next Pointer Register
#define US_TNCR         (284)	// Transmit Next Counter Register
#define US_PTCR         (288)	// PDC Transfer Control Register
#define US_PTSR         (292)	// PDC Transfer Status Register
// -------- US_CR : (USART Offset: 0x0) Debug Unit Control Register -------- 
#define AT91C_US_STTBRK           (0x1 <<  9)	// (USART) Start Break
#define AT91C_US_STPBRK           (0x1 << 10)	// (USART) Stop Break
#define AT91C_US_STTTO            (0x1 << 11)	// (USART) Start Time-out
#define AT91C_US_SENDA            (0x1 << 12)	// (USART) Send Address
#define AT91C_US_RSTIT            (0x1 << 13)	// (USART) Reset Iterations
#define AT91C_US_RSTNACK          (0x1 << 14)	// (USART) Reset Non Acknowledge
#define AT91C_US_RETTO            (0x1 << 15)	// (USART) Rearm Time-out
#define AT91C_US_DTREN            (0x1 << 16)	// (USART) Data Terminal ready Enable
#define AT91C_US_DTRDIS           (0x1 << 17)	// (USART) Data Terminal ready Disable
#define AT91C_US_RTSEN            (0x1 << 18)	// (USART) Request to Send enable
#define AT91C_US_RTSDIS           (0x1 << 19)	// (USART) Request to Send Disable
// -------- US_MR : (USART Offset: 0x4) Debug Unit Mode Register -------- 
#define AT91C_US_USMODE           (0xF <<  0)	// (USART) Usart mode
#define 	AT91C_US_USMODE_NORMAL               (0x0)	// (USART) Normal
#define 	AT91C_US_USMODE_RS485                (0x1)	// (USART) RS485
#define 	AT91C_US_USMODE_HWHSH                (0x2)	// (USART) Hardware Handshaking
#define 	AT91C_US_USMODE_MODEM                (0x3)	// (USART) Modem
#define 	AT91C_US_USMODE_ISO7816_0            (0x4)	// (USART) ISO7816 protocol: T = 0
#define 	AT91C_US_USMODE_ISO7816_1            (0x6)	// (USART) ISO7816 protocol: T = 1
#define 	AT91C_US_USMODE_IRDA                 (0x8)	// (USART) IrDA
#define 	AT91C_US_USMODE_SWHSH                (0xC)	// (USART) Software Handshaking
#define AT91C_US_CLKS             (0x3 <<  4)	// (USART) Clock Selection (Baud Rate generator Input Clock
#define 	AT91C_US_CLKS_CLOCK                (0x0 <<  4)	// (USART) Clock
#define 	AT91C_US_CLKS_FDIV1                (0x1 <<  4)	// (USART) fdiv1
#define 	AT91C_US_CLKS_SLOW                 (0x2 <<  4)	// (USART) slow_clock (ARM)
#define 	AT91C_US_CLKS_EXT                  (0x3 <<  4)	// (USART) External (SCK)
#define AT91C_US_CHRL             (0x3 <<  6)	// (USART) Clock Selection (Baud Rate generator Input Clock
#define 	AT91C_US_CHRL_5_BITS               (0x0 <<  6)	// (USART) Character Length: 5 bits
#define 	AT91C_US_CHRL_6_BITS               (0x1 <<  6)	// (USART) Character Length: 6 bits
#define 	AT91C_US_CHRL_7_BITS               (0x2 <<  6)	// (USART) Character Length: 7 bits
#define 	AT91C_US_CHRL_8_BITS               (0x3 <<  6)	// (USART) Character Length: 8 bits
#define AT91C_US_SYNC             (0x1 <<  8)	// (USART) Synchronous Mode Select
#define AT91C_US_NBSTOP           (0x3 << 12)	// (USART) Number of Stop bits
#define 	AT91C_US_NBSTOP_1_BIT                (0x0 << 12)	// (USART) 1 stop bit
#define 	AT91C_US_NBSTOP_15_BIT               (0x1 << 12)	// (USART) Asynchronous (SYNC=0) 2 stop bits Synchronous (SYNC=1) 2 stop bits
#define 	AT91C_US_NBSTOP_2_BIT                (0x2 << 12)	// (USART) 2 stop bits
#define AT91C_US_MSBF             (0x1 << 16)	// (USART) Bit Order
#define AT91C_US_MODE9            (0x1 << 17)	// (USART) 9-bit Character length
#define AT91C_US_CKLO             (0x1 << 18)	// (USART) Clock Output Select
#define AT91C_US_OVER             (0x1 << 19)	// (USART) Over Sampling Mode
#define AT91C_US_INACK            (0x1 << 20)	// (USART) Inhibit Non Acknowledge
#define AT91C_US_DSNACK           (0x1 << 21)	// (USART) Disable Successive NACK
#define AT91C_US_MAX_ITER         (0x1 << 24)	// (USART) Number of Repetitions
#define AT91C_US_FILTER           (0x1 << 28)	// (USART) Receive Line Filter
// -------- US_IER : (USART Offset: 0x8) Debug Unit Interrupt Enable Register -------- 
#define AT91C_US_RXBRK            (0x1 <<  2)	// (USART) Break Received/End of Break
#define AT91C_US_TIMEOUT          (0x1 <<  8)	// (USART) Receiver Time-out
#define AT91C_US_ITERATION        (0x1 << 10)	// (USART) Max number of Repetitions Reached
#define AT91C_US_NACK             (0x1 << 13)	// (USART) Non Acknowledge
#define AT91C_US_RIIC             (0x1 << 16)	// (USART) Ring INdicator Input Change Flag
#define AT91C_US_DSRIC            (0x1 << 17)	// (USART) Data Set Ready Input Change Flag
#define AT91C_US_DCDIC            (0x1 << 18)	// (USART) Data Carrier Flag
#define AT91C_US_CTSIC            (0x1 << 19)	// (USART) Clear To Send Input Change Flag
// -------- US_IDR : (USART Offset: 0xc) Debug Unit Interrupt Disable Register -------- 
// -------- US_IMR : (USART Offset: 0x10) Debug Unit Interrupt Mask Register -------- 
// -------- US_CSR : (USART Offset: 0x14) Debug Unit Channel Status Register -------- 
#define AT91C_US_RI               (0x1 << 20)	// (USART) Image of RI Input
#define AT91C_US_DSR              (0x1 << 21)	// (USART) Image of DSR Input
#define AT91C_US_DCD              (0x1 << 22)	// (USART) Image of DCD Input
#define AT91C_US_CTS              (0x1 << 23)	// (USART) Image of CTS Input

#endif
