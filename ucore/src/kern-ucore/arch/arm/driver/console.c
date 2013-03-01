#include <types.h>
#include <arm.h>
#include <board.h>
#include <stdio.h>
#include <kio.h>
#include <kdebug.h>
#include <string.h>
#include <sync.h>
#include <serial.h>

#ifdef HAS_SDS
#include <intel_sds.h>
#endif

#ifndef HAS_SDS
#define USE_UART
#endif

/* *
 * Here we manage the console input buffer, where we stash characters
 * received from the keyboard or serial port whenever the corresponding
 * interrupt occurs.
 * */

#define CONSBUFSIZE 512

static struct {
	uint8_t buf[CONSBUFSIZE];
	uint32_t rpos;
	uint32_t wpos;
} cons;

/* *
 * cons_intr - called by device interrupt routines to feed input
 * characters into the circular console input buffer.
 * */
static void cons_intr(int (*proc) (void))
{
	int c;
	while ((c = (*proc) ()) != -1) {
		if (c != 0) {
			cons.buf[cons.wpos++] = c;
			if (cons.wpos == CONSBUFSIZE) {
				cons.wpos = 0;
			}
		}
	}
}

/* serial_intr - try to feed input characters from serial port */
static void serial_intr(void)
{
	if (serial_check()) {
		cons_intr(serial_proc_data);
	}
}

#ifdef HAS_SDS
static int sds_proc_data_stdio()
{
	return sds_proc_data(0);
}

static void sds_intr(void)
{
	if (check_sds()) {
		cons_intr(sds_proc_data_stdio);
	}
}
#endif

// Originally in i386: Keyboard section
// Not implemented

/* cons_init - initializes the console devices */
void cons_init(void)
{
	if (!serial_check()) {
		kprintf("serial port does not exist!!\n");
	}
}

/* cons_putc - print a single character @c to console devices */
// serial only atm
void cons_putc(int c)
{
	bool intr_flag;
	local_intr_save(intr_flag);
	{
#ifdef USE_UART
		serial_putc(c);
#endif
#ifdef HAS_SDS
		if (check_sds() && is_debugging())
			sds_poll_proc();
		sds_putc(0, c);
#endif
	}
	local_intr_restore(intr_flag);
}

/* *
 * cons_getc - return the next input character from console,
 * or 0 if none waiting.
 * */
int cons_getc(void)
{
	int c = 0;
	bool intr_flag;
	local_intr_save(intr_flag);
	{
		// poll for any pending input characters,
		// so that this function works even when interrupts are disabled
		// (e.g., when called from the kernel monitor).
#ifdef USE_UART
		serial_intr();
#endif
#ifdef HAS_SDS
		if (check_sds() && is_debugging())
			sds_poll_proc();
		sds_intr();
#endif
		//kbd_intr();

		// grab the next character from the input buffer.
		if (cons.rpos != cons.wpos) {
			c = cons.buf[cons.rpos++];
			if (cons.rpos == CONSBUFSIZE) {
				cons.rpos = 0;
			}
		}
	}
	local_intr_restore(intr_flag);
	return c;
}
