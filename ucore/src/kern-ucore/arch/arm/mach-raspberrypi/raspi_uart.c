#include <types.h>
#include <arm.h>
#include <stdio.h>
#include <kio.h>
#include <string.h>
#include <sync.h>
#include <board.h>
#include <assert.h>
#include <kio.h>
#include <barrier.h>

#define UART1_BASE 0x20215000
#define AUX_MU_IO_REG (UART1_BASE+0x40)
#define AUX_MU_IIR_REG (UART1_BASE+0x48)
#define AUX_MU_LSR_REG (UART1_BASE+0x54)
#define AUX_MU_LSR_REG_TX_EMPTY 0x20
#define AUX_MU_LSR_REG_DATA_READY 0x01

static bool serial_exists = 0;

void
serial_init_early() {
  if(serial_exists)
    return ;
  kprintf("Serial init skipped: already initialized in bootloader\n");
  serial_exists = 1;
}

void
serial_init_mmu() {
  // make address mapping
  //UART1_BASE is within device address range,
  //therefore ioremap is not necessary for uart
}

static void
serial_putc_sub(int c) {
  dmb();
  while(!(inb(AUX_MU_LSR_REG)&AUX_MU_LSR_REG_TX_EMPTY)) ;
  dmb();
  outb(AUX_MU_IO_REG, c);
  dmb();
}

/* serial_putc - print character to serial port */
void
serial_putc(int c) {
    if (c == '\b') {
        serial_putc_sub('\b');
        serial_putc_sub(' ');
        serial_putc_sub('\b');
    }
    else if (c == '\n') {
        serial_putc_sub('\r');
        serial_putc_sub('\n');
    }
    else {
      serial_putc_sub(c);
    }
}

/* serial_proc_data - get data from serial port */
int
serial_proc_data(void) {
}


int serial_check(){
  return serial_exists;
}

void serial_clear(){
}
