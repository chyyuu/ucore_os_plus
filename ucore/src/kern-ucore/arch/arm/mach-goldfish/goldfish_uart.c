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
static uint32_t uart_base = GOLDFISH_UART0;
/* from qemu-android */
enum {
	TTY_PUT_CHAR = 0x00,
	TTY_BYTES_READY = 0x04,
	TTY_CMD = 0x08,

	TTY_DATA_PTR = 0x10,
	TTY_DATA_LEN = 0x14,

	TTY_CMD_INT_DISABLE = 0,
	TTY_CMD_INT_ENABLE = 1,
	TTY_CMD_WRITE_BUFFER = 2,
	TTY_CMD_READ_BUFFER = 3,
};

static uint8_t tty_buffer[4];

static int serial_int_handler(int irq, void *data)
{
	extern void dev_stdin_write(char c);
	char c = cons_getc();
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
	uart_base = base;
	/* buffer size = 1 */
	outw(uart_base + TTY_DATA_LEN, 1);
	outw(uart_base + TTY_DATA_PTR, (uint32_t) tty_buffer);
	/* turn on recv interrput */
	outw(uart_base + TTY_CMD, TTY_CMD_INT_ENABLE);
	//serial_clear();
	register_irq(irq, serial_int_handler, NULL);
	pic_enable(irq);

}

static void serial_putc_sub(int c)
{
	//if(serial_exists)
	outb(uart_base + TTY_PUT_CHAR, c);
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
	//kprintf("%08x\n", inw(GOLDFISH_UART0+PL011_UARTFR));
	/*
	   if (inw(GOLDFISH_UART0 + PL011_UARTFR) & PL011_RXFE) {
	   return -1;
	   }
	   int c = inb(GOLDFISH_UART0);
	   if (c == 127) {
	   c = '\b';
	   } */
	if (inw(uart_base + TTY_BYTES_READY) == 0) {
		return -1;
	}
	outw(uart_base + TTY_CMD, TTY_CMD_READ_BUFFER);
	if (tty_buffer[0] == 127)
		tty_buffer[0] = '\b';
	return tty_buffer[0];
}

int serial_check()
{
	return serial_exists;
}

void serial_clear()
{
}
