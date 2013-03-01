#include <types.h>
#include <nios2_timer.h>
#include <nios2.h>
#include <system.h>
#include <alt_types.h>
#include <altera_avalon_timer_regs.h>
#include <clock.h>
#include <stdio.h>
#include <nios2_irq.h>

struct np_timer *na_timer = (struct np_timer *)(0xE0000000 + TIMER_BASE);
struct np_timer *na_timer1 = (struct np_timer *)(0xE0000000 + TIMER_US_BASE);

/* timer_init() */
void nios2_timer_init(void)
{
	IOWR_ALTERA_AVALON_TIMER_CONTROL(na_timer,
					 ALTERA_AVALON_TIMER_CONTROL_ITO_MSK |
					 ALTERA_AVALON_TIMER_CONTROL_CONT_MSK |
					 ALTERA_AVALON_TIMER_CONTROL_START_MSK);
	alt_irq_enable(TIMER_IRQ);
	kprintf("++ setup timer interrupts\n");
}

void nios2_timer_usleep(int usec)
{
	int period = 50 * usec;
	IOWR_ALTERA_AVALON_TIMER_PERIODL(na_timer1, period & 0xFFFF);
	IOWR_ALTERA_AVALON_TIMER_PERIODH(na_timer1, (period >> 16) & 0xFFFF);
	IOWR_ALTERA_AVALON_TIMER_STATUS(na_timer1, 0);
	IOWR_ALTERA_AVALON_TIMER_CONTROL(na_timer1,
					 ALTERA_AVALON_TIMER_CONTROL_START_MSK);
	while (1) {
		if (IORD_ALTERA_AVALON_TIMER_STATUS(na_timer1) &
		    ALTERA_AVALON_TIMER_STATUS_TO_MSK) {
			break;
		}
	}
}
