#include <arm.h>
#include <board.h>
#include <clock.h>
#include <trap.h>
#include <stdio.h>
#include <kio.h>
#include <picirq.h>

static void reload_timer()
{
}

void clock_clear(void)
{
}

static int clock_int_handler(int irq, void * data)
{
}

void clock_init_arm(uint32_t base, int irq)
{
}
