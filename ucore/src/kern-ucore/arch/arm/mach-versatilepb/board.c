/*
 * =====================================================================================
 *
 *       Filename:  board.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/24/2012 01:21:05 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <board.h>
#include <serial.h>
#include <clock.h>

static const char *message = "Initializing VersatilePB Board...\n";

static void put_string(const char *str)
{
	while (*str)
		serial_putc(*str++);
}

void board_init()
{
	put_string(message);
	pic_init();		// init interrupt controller
	extern void serial_init();
	serial_init();

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
