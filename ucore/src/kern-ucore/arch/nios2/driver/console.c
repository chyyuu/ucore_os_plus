#include <system.h>
#include <io.h>
#include <nios2_irq.h>

#define JTAG_UART_DATA ((volatile int*) (JTAG_UART_BASE + 0xE0000000))
#define JTAG_UART_CONTROL ((volatile int*) (JTAG_UART_BASE + 0xE0000000 + 4))

#define UART_DATA ((volatile int*) (UART_BASE + 0xD0000000))
#define UART_CONTROL ((volatile int*) (UART_BASE + 0xD0000000 + 4))

static void jtag_uart_putc_sub(int c)
{
	while (1) {
		//if room in output buffer
		if (IORD(JTAG_UART_CONTROL, 0) & 0xffff0000) {
			//then write the next character
			IOWR(JTAG_UART_DATA, 0, c);
			break;
		}
	}
}

/*
static void
uart_putc_sub(int c) {
    while(1)
    {
         //if room in output buffer
         //if(IORD(UART_CONTROL, 0)&0xffff0000  ) 
         //{
            //then write the next character
            IOWR(UART_DATA, 0, c);
            break;
         //}
     }	
}
*/

/* jtag_uart_putc - print character to jtag uart port */
static void jtag_uart_putc(int c)
{
	if (c != '\b') {
		jtag_uart_putc_sub(c);
	} else {
		jtag_uart_putc_sub('\b');
		jtag_uart_putc_sub(' ');
		jtag_uart_putc_sub('\b');
	}
}

/*
static void
uart_putc(int c) {
	if (c != '\b') {
		uart_putc_sub(c);
	}
	else {
		uart_putc_sub('\b');
		uart_putc_sub(' ');
		uart_putc_sub('\b');
	}
}
*/

/* cons_init - initializes the console devices */
void cons_init(void)
{
	//int ctl = IORD(JTAG_UART_CONTROL, 0);
	IOWR(JTAG_UART_CONTROL, 0, 1);

	alt_irq_enable(JTAG_UART_IRQ);
}

/* cons_putc - print a single character @c to console devices */
void cons_putc(int c)
{
	jtag_uart_putc(c);
	//uart_putc(c);TODO: uart
}

/* *
 * cons_getc - return the next input character from console,
 * or 0 if none waiting.
 * */
int cons_getc(void)
{
	int ret = IORD(JTAG_UART_DATA, 0);
	if (!(ret & (1 << 15)))
		ret = 0;
	else
		ret = ret & 0xff;
	return ret;
}
