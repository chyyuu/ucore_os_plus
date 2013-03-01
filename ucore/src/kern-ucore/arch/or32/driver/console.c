#include <console.h>
#include <uart.h>

/* serial_intr - try to feed input characters from serial port */
void serial_intr(void)
{
	/*! TODO */
}

/* kbd_intr - try to feed input characters from keyboard */
void kbd_intr(void)
{
	/*! TODO */
}

/* cons_init - initializes the console devices */
void cons_init(void)
{
	uart_init();
}

/* cons_putc - print a single character @c to console devices */
void cons_putc(int c)
{
	uart_putc(c);
	asm("l.nop; l.nop;");
}

/* *
 * cons_getc - return the next input character from console,
 * or 0 if none waiting.
 * */
int cons_getc(void)
{
	return uart_getc();
}
