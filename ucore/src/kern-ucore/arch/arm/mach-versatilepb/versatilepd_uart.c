/*
 * =====================================================================================
 *
 *       Filename:  versatilepd_uart.c
 *
 *    Description:  PL011 driver
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
#include <kio.h>
#include <string.h>
#include <sync.h>
#include <board.h>
#include <picirq.h>

static bool serial_exists = 0;

void serial_init(void)
{
	if (serial_exists)
		return;
	serial_exists = 1;
	/* turn on recv interrput */
	outw(VERSATILEPB_UART0 + UARTIMSC, UART_RXIM);
	//serial_clear();

	pic_enable(UART_IRQ);

}

static void serial_putc_sub(int c)
{
	outb(VERSATILEPB_UART0, c);
}

/* serial_putc - print character to serial port */
void serial_putc(int c)
{
	if (c != '\b') {
		serial_putc_sub(c);
	} else {
		serial_putc_sub('\b');
		serial_putc_sub(' ');
		serial_putc_sub('\b');
	}
}

/* serial_proc_data - get data from serial port */
int serial_proc_data(void)
{
	//kprintf("%08x\n", inw(VERSATILEPB_UART0+PL011_UARTFR));
	if (inw(VERSATILEPB_UART0 + PL011_UARTFR) & PL011_RXFE) {
		return -1;
	}
	int c = inb(VERSATILEPB_UART0);
	if (c == 127) {
		c = '\b';
	}
	return c;
}

int serial_check()
{
	return serial_exists;
}

void serial_clear()
{
	outw(VERSATILEPB_UART0 + UARTICR, UART_RXIM);
}
