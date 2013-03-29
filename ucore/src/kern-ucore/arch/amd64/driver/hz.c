/*
 * =====================================================================================
 *
 *       Filename:  hz.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/20/2013 07:24:16 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <arch.h>
#include <hz.h>

#define IO_TIMER1       0x040           // 8253 Timer #1
#define TIMER_FREQ      1193182
#define	TIMER_CNTR      (IO_TIMER1 + 0)	// timer counter port
#define TIMER_MODE      (IO_TIMER1 + 3) // timer mode port
#define TIMER_SEL0      0x00    // select counter 0
#define TIMER_TCOUNT    0x00    // mode 0, terminal count
#define TIMER_16BIT     0x30    // r/w counter 16 bits, LSB first
#define TIMER_STAT      0xe0    // read status mode
#define TIMER_STAT0     (TIMER_STAT | 0x2)  // status mode counter 0

uint64_t cpuhz;

void
microdelay(uint64_t delay)
{
  uint64_t tscdelay = (cpuhz * delay) / 1000000;
  uint64_t s = rdtsc();
  while (rdtsc() - s < tscdelay)
    nop_pause();
}

void
hz_init(void)
{
  // Setup PIT for terminal count starting from 2^16 - 1
  uint64_t xticks = 0x000000000000FFFFull;
  outb(TIMER_MODE, TIMER_SEL0 | TIMER_TCOUNT | TIMER_16BIT);  
  outb(IO_TIMER1, xticks % 256);
  outb(IO_TIMER1, xticks / 256);

  // Wait until OUT bit of status byte is set
  uint64_t s = rdtsc();
  do {
    outb(TIMER_MODE, TIMER_STAT0);
    if (rdtsc() - s > 1ULL<<32) {
      kprintf("inithz: PIT stuck, assuming 2GHz\n");
      cpuhz = 2 * 1000 * 1000 * 1000;
      return;
    }
  } while (!(inb(TIMER_CNTR) & 0x80));
  uint64_t e = rdtsc();

  cpuhz = ((e-s)*10000000) / ((xticks*10000000)/TIMER_FREQ);
}
