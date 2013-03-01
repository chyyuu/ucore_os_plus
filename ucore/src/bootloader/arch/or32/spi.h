#ifndef __ARCH_OR32_DRIVERS_SPI_H__
#define __ARCH_OR32_DRIVERS_SPI_H__

#include <board.h>

#ifndef __ASSEMBLER__
#include <types.h>

#define SPI_BASE   (0xf0000000 + (SPI_PHYSICAL_BASE >> 4))
#define BLOCK_SIZE 512		/* In byte */

struct partition_info_entry {
	uint8_t flag;
	uint8_t padding1;
	uint32_t start;		/* in block */
	uint32_t size;		/* in block */
	uint32_t padding2;
	uint16_t padding3;
} __attribute__ ((packed));

#define PARTITION_ENTRY_FLAG_BOOTABLE 0x80	/* The block contains a valid kernel elf. */
#define PARTITION_ENTRY_FLAG_VALID    0x1

#define MAX_PARTITION_NUM             4

#endif /* !__ASSEMBLER__ */

/**
 * Register definitions
 */
#define SPI_MASTER_VERSION_REG     0x0
#define SPI_MASTER_CONTROL_REG     0x1
#define SPI_TRANS_TYPE_REG         0x2
#define SPI_TRANS_CTRL_REG         0x3
#define SPI_TRANS_STS_REG          0x4
#define SPI_TRANS_ERROR_REG        0x5
#define SPI_DIRECT_ACCESS_DATA_REG 0x6
#define SPI_SD_ADDR_7_0_REG        0x7
#define SPI_SD_ADDR_15_8_REG       0x8
#define SPI_SD_ADDR_23_16_REG      0x9
#define SPI_SD_ADDR_31_24_REG      0xa
#define SPI_CLK_DEL_REG            0xb
#define SPI_RX_FIFO_DATA_REG       0x10
#define SPI_RX_FIFO_DATA_COUNT_MSB 0x12
#define SPI_RX_FIFO_DATA_COUNT_LSB 0x13
#define SPI_RX_FIFO_CONTROL_REG    0x14
#define SPI_TX_FIFO_DATA_REG       0x20
#define SPI_TX_FIFO_CONTROL_REG    0x24

/**
 * Version Register
 */
#define SPI_VERSION_NUM_MAJOR_MASK  0xf0
#define SPI_VERSION_NUM_MINOR_MASK  0x0f

/**
 * Reset logic
 */
#define SPI_RST  0x1

/**
 * Transaction Type Register
 */
#define SPI_DIRECT_ACCESS      0
#define SPI_INIT_SD            1
#define SPI_RW_READ_SD_BLOCK   2
#define SPI_RW_WRITE_SD_BLOCK  3

/**
 * Transaction Control Register
 */
#define SPI_TRANS_START        1

/**
 * Transaction Status Register
 */
#define SPI_TRANS_BUSY         1

/**
 * Transaction Error Register
 */
#define SPI_SD_INIT_ERROR      0x3
#define SPI_SD_READ_ERROR      0xc
#define SPI_SD_WRITE_ERROR     0x30

#define SPI_NO_ERROR           0
#define SPI_INIT_CMD0_ERROR    0x1
#define SPI_INIT_CMD1_ERROR    0x2
#define SPI_READ_CMD_ERROR     0x4
#define SPI_READ_TOKEN_ERROR   0x8
#define SPI_WRITE_CMD_ERROR    0x10
#define SPI_WRITE_DATA_ERROR   0x20
#define SPI_WRITE_BUSY_ERROR   0x30

#endif /* __ARCH_OR32_DRIVERS_SPI_H__ */
