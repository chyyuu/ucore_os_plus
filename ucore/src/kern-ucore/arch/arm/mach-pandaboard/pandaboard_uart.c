/*
 * =====================================================================================
 *
 *       Filename:  versatilepd_uart.c
 *
 *    Description:  NS16550
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
#include <pmm.h>

#define UART_TX 0x00
#define UART_RX 0x00
#define UART_LSR 0x14
#define UART_IER 0x04
#define UART_IIR 0x08
#define UART_LSR_SR_E (1<<5)
#define UART_LSR_DR	0x01	/* Receiver data ready */

#define UART_IER_RHR_IT (1<<0)

static bool serial_exists = 0;
static uint32_t uart_base = PANDABOARD_UART0;

static void serial_clear()
{
}

static int serial_int_handler(int irq, void *data)
{
	extern void dev_stdin_write(char c);
	char c = cons_getc();
	//serial_putc(c);
	dev_stdin_write(c);
	return 0;
}

void serial_init_early()
{
}

void serial_init(uint32_t base, uint32_t irq)
{
	if (serial_exists)
		return;
	serial_exists = 1;

	void *newbase = __ucore_ioremap(base, PGSIZE, 0);
	uart_base = (uint32_t) newbase;

	outw(uart_base + UART_IER, UART_IER_RHR_IT);
	register_irq(irq, serial_int_handler, NULL);
	pic_enable(irq);

}

static void serial_putc_sub(int c)
{
	//if(serial_exists)
	while ((inw(uart_base + UART_LSR) & UART_LSR_SR_E) == 0) ;
	if (c == '\n')
		outw(uart_base + UART_TX, '\r');
	outw(uart_base + UART_TX, c & 0xff);
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
	//kprintf("%08x\n", inw(PANDABOARD_UART0+PL011_UARTFR));
	if ((inw(uart_base + UART_LSR) & UART_LSR_DR) == 0) {
		return -1;
	}
	int c = inw(uart_base + UART_RX);
	if (c == 127) {
		c = '\b';
	}
	return c;
}

int serial_check()
{
	return serial_exists;
}
