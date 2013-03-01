#include <board.h>
#include <picirq.h>
#include <serial.h>
#include <clock.h>

static const char *message = "Initializing Raspberry Pi Board...\n";

static void put_string(const char *str)
{
	while (*str)
		serial_putc(*str++);
}

void board_init_early()
{
	put_string(message);

	pic_init();		// init interrupt controller
	// fixed base and irq
	clock_init_arm(0, 0);	// linux put tick_init in kernel_main, so do we~

	// init serial
	serial_init_early();
}

void board_init()
{
	serial_init_mmu();
}

/* no nand */
int check_nandflash()
{
	return 0;
}

struct nand_chip *get_nand_chip()
{
	return NULL;
}
