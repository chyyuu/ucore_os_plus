/*
 * =====================================================================================
 *
 *       Filename:  versatilepd_uart.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/17/2012 02:43:34 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <types.h>
#include <arm.h>
#include <stdio.h>
#include <string.h>
#include <sync.h>
#include <board.h>

static int serial_exists = 0;

void serial_init(void)
{
	if (serial_exists)
		return;
	serial_exists = 1;
	/*
	 * Disable interrupts 
	 */
	outw(AT91SAM_DBGU_BASE + US_IDR, -1);
	/*
	 * Enable RX and Tx 
	 */
	outw(AT91SAM_DBGU_BASE + US_CR, AT91C_US_RXEN | AT91C_US_TXEN);

	/* Enable Recv Interrupt */
	outw(AT91SAM_DBGU_BASE + DBGU_IER, AT91C_US_RXRDY);
	pic_enable(AT91C_ID_SYS);
}

static void serial_putc_sub(int c)
{
	while (!(inw(AT91SAM_DBGU_BASE + US_CSR) & AT91C_US_TXRDY)) ;
	outb(AT91SAM_DBGU_BASE + US_THR, c);
}

/* serial_putc - print character to serial port */
void serial_putc(int c)
{
	//TODO dirty hack for minicom
	switch (c & 0xff) {
	case '\b':
		serial_putc_sub('\b');
		serial_putc_sub(' ');
		serial_putc_sub('\b');
		break;
	case '\n':
		serial_putc_sub('\r');
		serial_putc_sub('\n');
		break;
	default:
		serial_putc_sub(c);

	}
}

/* serial_proc_data - get data from serial port */
int serial_proc_data(void)
{
	if (!(inw(AT91SAM_DBGU_BASE + US_CSR) & AT91C_US_RXRDY))
		return -1;
	int ret = inw(AT91SAM_DBGU_BASE + US_RHR) & 0xff;
	//serial_putc(ret);
	return ret;
}

int serial_check()
{
	return serial_exists;
}

void serial_clear()
{
	outw(AT91SAM_DBGU_BASE + US_CR, AT91C_US_RSTSTA);
}
