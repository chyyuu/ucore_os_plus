/*
 * =====================================================================================
 *
 *       Filename:  irq_handler.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/25/2012 08:33:46 PM
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
#include <picirq.h>
#include <clock.h>
#include <intr.h>
#include <kio.h>
#include <memlayout.h>
#include <board.h>
#include <clock.h>
#include <serial.h>

extern void dev_stdin_write(char c);

void irq_handler(void)
{
	// panic("NOT imple");
	uint32_t status = inw(AT91C_BASE_AIC + AIC_ISR);
	uint32_t pending = inw(AT91C_BASE_AIC + AIC_IPR);
	// kprintf(".. %08x %08x %08x\n", status, pending, inw(AT91C_BASE_AIC+AIC_IMR));
	//kprintf("$");
	if (pending & (1 << AT91C_ID_SYS)) {
		//kprintf("#");
		if (inw(AT91C_BASE_PIT + PIT_SR) & 0x01) {	//PIT
			ticks++;
			/* hw polling */
#ifdef HAS_SDS
			extern int sds_poll_proc();
			if (check_sds()) {
				sds_poll_proc();
				//drain buffer
				//char c;
				//while(1){
				char c = cons_getc();
				//if(c <= 0) break;
				if (c > 0)
					dev_stdin_write(c);
				//}
			}
#endif

			//schedule
			if (ticks % 5 == 0)
				run_timer_list();
			clock_clear();
		}
		status = inw(AT91SAM_DBGU_BASE + US_CSR);
		if (status & AT91C_US_RXRDY) {
			char c = cons_getc();
			dev_stdin_write(c);
			//kprintf("#");
			serial_clear();
		}
		irq_clear(AT91C_ID_SYS);
	}
	outw(AT91C_BASE_AIC + AIC_EOICR, ~0);

}
