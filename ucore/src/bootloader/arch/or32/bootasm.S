/**************************************************
 * This is the assembly part of or32-ucore on-disk bootloader.
 * This is here just to make sure that the very first position is the entry of the loader.
 * Simply set up the stack and jump to the C code.
 **************************************************/

#include <board.h>
#include <spi.h>

.macro	load32i	reg, const
		l.movhi	\reg, hi(\const)
		l.ori	\reg, \reg, lo(\const)
.endm

		.section .entry, "ax"

_entry:
		// Prepare the stack for boot-time use.
		load32i	r1, 0x1ffe00

		// Prepare the first argument
		load32i	r3, 0x1fffbe
		
		// Jump to the main loader (never return)
		l.jal	bootmain
		l.nop

/**
 * Read a block from the sd card.
 * @param dst(r3)     the buffer where the data read is placed.
 * @param blkno(r4)   the block number
 */

		.globl 	_readblock
_readblock:
		l.movhi	r17, hi(SPI_PHYSICAL_BASE)
		l.slli	r4, r4, 9
		/* Load the address. */
		l.sb	SPI_SD_ADDR_7_0_REG(r17), r4
		l.srli	r4, r4, 8
		l.sb	SPI_SD_ADDR_15_8_REG(r17), r4
		l.srli	r4, r4, 8
		l.sb	SPI_SD_ADDR_23_16_REG(r17), r4
		l.srli	r4, r4, 8
		l.sb	SPI_SD_ADDR_31_24_REG(r17), r4

		l.addi	r4, r0, SPI_RW_READ_SD_BLOCK
		l.sb	SPI_TRANS_TYPE_REG(r17), r4
		l.addi	r4, r0, SPI_TRANS_START
		l.sb	SPI_TRANS_CTRL_REG(r17), r4

		/* Wait till it's done. */
		l.addi	r19, r0, SPI_TRANS_BUSY
1:
		l.lbz	r4, SPI_TRANS_STS_REG(r17)
		l.sfeq	r4, r19
		l.bf 	1b
		l.nop

		/* Check whether everything's right. */
		l.lbz	r4, SPI_TRANS_ERROR_REG(r17)
		l.andi	r4, r4, SPI_SD_READ_ERROR
		l.sfeq 	r4, r0
		l.bnf	out
		l.nop

		l.addi 	r4, r3, 512
2:
		l.lbz	r21, SPI_RX_FIFO_DATA_REG(r17)
		l.sb	0(r3), r21
		l.addi	r3, r3, 1
		l.sfeq	r3, r4
		l.bnf	2b
		l.nop

out:
		l.jr	r9
		l.nop