#ifndef __ARCH_OR32_DRIVERS_BOARD_H__
#define __ARCH_OR32_DRIVERS_BOARD_H__

//#define BOARD

/* UART base address */
#ifdef BOARD
#define UART_BAUD_RATE 	         9600
#else
#define UART_BAUD_RATE 	         115200
#endif
#define UART_PHYSICAL_BASE  	 0x50000000
#define UART_IRQ                 2

/* Clock Frequence */
#define IN_CLK                   50000000	// 50MHz
#ifdef BOARD
#define TIMER_FREQ               0x400000
#else
#define TIMER_FREQ               0x40000
#endif

/* RAM size */
#ifdef BOARD
#define RAM_SIZE                 0x4000000	// 64M bytes
#else
#define RAM_SIZE                 0x200000	// 2M bytes
#endif

#define DISK_FS_BASE             0x800000
#define DISK_FS_SIZE             0x400000
#define SWAP_FS_BASE             0xc00000
#define SWAP_FS_SIZE             0x400000

#define GPIO_PHYSICAL_BASE       0x20000000
#define SPI_PHYSICAL_BASE        0x70000000

#endif /* __ARCH_OR32_DRIVERS_BOARD_H__ */
