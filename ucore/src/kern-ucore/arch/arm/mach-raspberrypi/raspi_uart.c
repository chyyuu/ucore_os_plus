#include <types.h>
#include <arm.h>
#include <stdio.h>
#include <kio.h>
#include <string.h>
#include <sync.h>
#include <board.h>
#include <assert.h>
#include <picirq.h>
#include <kio.h>
#include <barrier.h>
#include <framebuffer.h>

#define UART1_BASE 0x20215000
#define AUX_MU_IO_REG (UART1_BASE+0x40)
#define AUX_MU_IIR_REG (UART1_BASE+0x48)
#define AUX_MU_LSR_REG (UART1_BASE+0x54)
#define AUX_MU_LSR_REG_TX_EMPTY 0x20
#define AUX_MU_LSR_REG_DATA_READY 0x01
#define UART1_IRQ 29

static bool serial_exists = 0;

static int serial_int_handler(int irq, void *data)
{
	extern void dev_stdin_write(char c);
	char c = cons_getc();
	dev_stdin_write(c);
	return 0;
}

void serial_init_early()
{
	if (serial_exists)
		return;
	kprintf("Serial init skipped: already initialized in bootloader\n");
	serial_exists = 1;

	fb_init();
}

void serial_init_mmu()
{
	// make address mapping
	//UART1_BASE is within device address range,
	//therefore ioremap is not necessary for uart

	// init interrupt
	register_irq(UART1_IRQ, serial_int_handler, NULL);
	pic_enable(UART1_IRQ);

	// init framebuffer (mmu)
	fb_init_mmu();
}

static void serial_putc_sub(int c)
{
	dmb();
	while (!(inb(AUX_MU_LSR_REG) & AUX_MU_LSR_REG_TX_EMPTY)) ;
	dmb();
	outb(AUX_MU_IO_REG, c);
	dmb();
	fb_write(c);
	dmb();
}

/* serial_putc - print character to serial port */
void serial_putc(int c)
{
	if (c == '\b') {
		serial_putc_sub('\b');
		serial_putc_sub(' ');
		serial_putc_sub('\b');
	} else if (c == '\n') {
		serial_putc_sub('\r');
		serial_putc_sub('\n');
	} else {
		serial_putc_sub(c);
	}
}

/* serial_proc_data - get data from serial port */
int serial_proc_data(void)
{
	// return -1 when no char is available at the moment
	//
	int rb = inb(AUX_MU_IIR_REG);
	if ((rb & 1) == 1) {
		return -1;	//no more interrupts
	} else if ((rb & 6) == 4) {
		while (!(inb(AUX_MU_LSR_REG) & AUX_MU_LSR_REG_DATA_READY)) ;
		int c = inb(AUX_MU_IO_REG);
		if (c == 127) {
			c = '\b';
		}
		/*
		   // echo
		   while(!(inb(AUX_MU_LSR_REG)&AUX_MU_LSR_REG_TX_EMPTY)) ;
		   outb(AUX_MU_IO_REG, c);
		 */
		return c;
	} else {
		panic("unexpected AUX_MU_IIR_REG = %02x\n", rb);
	}
}

int serial_check()
{
	return serial_exists;
}

void serial_clear()
{
}
