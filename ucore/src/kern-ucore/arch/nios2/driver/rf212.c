/*
 * "Hello World" example.
 *
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example
 * designs. It runs with or without the MicroC/OS-II RTOS and requires a STDOUT
 * device in your system's hardware.
 * The memory footprint of this hosted application is ~69 kbytes by default
 * using the standard reference design.
 *
 * For a reduced footprint version of this template, and an explanation of how
 * to reduce the memory footprint for a given application, see the
 * "small_hello_world" template.
 *
 */

#include <stdio.h>
#include <altera_avalon_spi_regs.h>
#include <system.h>
#include <assert.h>
#include <nios2.h>
#include <altera_avalon_pio_regs.h>
#include <rf212.h>

#define WRITE 1
#define READ 2

static uint8_t irq_status;
static uint8_t trx_status;

#define SPI_BASE_KERNEL (SPI_BASE + 0xE0000000)
#define RF212_IRQ_BASE_KERNEL (RF212_IRQ_BASE + 0xE0000000)
#define RF212_SLP_TR_BASE_KERNEL (RF212_SLP_TR_BASE + 0xE0000000)

uint8_t rf212_register(int mode, uint8_t regnum, uint8_t value)
{
	uint8_t cmd = 0;
	if (mode == WRITE) {
		cmd = 0xc0 | (regnum & 0x3F);
	} else {
		cmd = 0x80 | (regnum & 0x3F);
	}

	IOWR_ALTERA_AVALON_SPI_SLAVE_SEL(SPI_BASE_KERNEL, 1);
	IOWR_ALTERA_AVALON_SPI_CONTROL(SPI_BASE_KERNEL, ALTERA_AVALON_SPI_CONTROL_SSO_MSK);
	IORD_ALTERA_AVALON_SPI_RXDATA(SPI_BASE_KERNEL);
	uint8_t tx[2] = {cmd, value};
	uint8_t rx[2];
	int i;
	uint32_t status;
	for (i = 0; i < 2; ++i) {
		do {
			status = IORD_ALTERA_AVALON_SPI_STATUS(SPI_BASE_KERNEL);
		} while ((status & ALTERA_AVALON_SPI_STATUS_TRDY_MSK) == 0);
		IOWR_ALTERA_AVALON_SPI_TXDATA(SPI_BASE_KERNEL, tx[i]);
		do {
			status = IORD_ALTERA_AVALON_SPI_STATUS(SPI_BASE_KERNEL);
		} while ((status & ALTERA_AVALON_SPI_STATUS_RRDY_MSK) == 0);
		rx[i] = (uint8_t)IORD_ALTERA_AVALON_SPI_RXDATA(SPI_BASE_KERNEL);
	}
	do {
		status = IORD_ALTERA_AVALON_SPI_STATUS(SPI_BASE_KERNEL);
	} while ((status & ALTERA_AVALON_SPI_STATUS_TMT_MSK) == 0);
	IOWR_ALTERA_AVALON_SPI_CONTROL(SPI_BASE_KERNEL, 0);
	return rx[1];
}

uint8_t rf212_rx(uint8_t *data)
{
	IOWR_ALTERA_AVALON_SPI_SLAVE_SEL(SPI_BASE_KERNEL, 1);
	IOWR_ALTERA_AVALON_SPI_CONTROL(SPI_BASE_KERNEL, ALTERA_AVALON_SPI_CONTROL_SSO_MSK);
	IORD_ALTERA_AVALON_SPI_RXDATA(SPI_BASE_KERNEL);
	uint8_t tx = 0x20;

	int i;
	uint32_t status;
	uint8_t len = 2;
	for (i = 0; i < len + 5; ++i) {
		do {
			status = IORD_ALTERA_AVALON_SPI_STATUS(SPI_BASE_KERNEL);
		} while ((status & ALTERA_AVALON_SPI_STATUS_TRDY_MSK) == 0);
		IOWR_ALTERA_AVALON_SPI_TXDATA(SPI_BASE_KERNEL, tx);
		do {
			status = IORD_ALTERA_AVALON_SPI_STATUS(SPI_BASE_KERNEL);
		} while ((status & ALTERA_AVALON_SPI_STATUS_RRDY_MSK) == 0);
		uint8_t rx = (uint8_t)IORD_ALTERA_AVALON_SPI_RXDATA(SPI_BASE_KERNEL);
		if (i == 1)
			len = rx;
		else if (i > 1)
			data[i - 2] = rx;
	}
	do {
		status = IORD_ALTERA_AVALON_SPI_STATUS(SPI_BASE_KERNEL);
	} while ((status & ALTERA_AVALON_SPI_STATUS_TMT_MSK) == 0);
	IOWR_ALTERA_AVALON_SPI_CONTROL(SPI_BASE_KERNEL, 0);
	return len;
}

void rf212_tx(uint8_t *data, uint8_t len)
{
	IOWR_ALTERA_AVALON_SPI_SLAVE_SEL(SPI_BASE_KERNEL, 1);
	IOWR_ALTERA_AVALON_SPI_CONTROL(SPI_BASE_KERNEL, ALTERA_AVALON_SPI_CONTROL_SSO_MSK);
	IORD_ALTERA_AVALON_SPI_RXDATA(SPI_BASE_KERNEL);

	int i;
	uint32_t status;
	for (i = 0; i < len + 4; ++i) {
		do {
			status = IORD_ALTERA_AVALON_SPI_STATUS(SPI_BASE_KERNEL);
		} while ((status & ALTERA_AVALON_SPI_STATUS_TRDY_MSK) == 0);
		uint8_t tx;
		if (i == 0)
			tx = 0x60;
		else if (i == 1)
			tx = len + 2;
		else if (i <= len + 1)
			tx = data[i - 2];
		else
			tx = 0;
		IOWR_ALTERA_AVALON_SPI_TXDATA(SPI_BASE_KERNEL, tx);
		do {
			status = IORD_ALTERA_AVALON_SPI_STATUS(SPI_BASE_KERNEL);
		} while ((status & ALTERA_AVALON_SPI_STATUS_RRDY_MSK) == 0);
		IORD_ALTERA_AVALON_SPI_RXDATA(SPI_BASE_KERNEL);
	}
	do {
		status = IORD_ALTERA_AVALON_SPI_STATUS(SPI_BASE_KERNEL);
	} while ((status & ALTERA_AVALON_SPI_STATUS_TMT_MSK) == 0);
	IOWR_ALTERA_AVALON_SPI_CONTROL(SPI_BASE_KERNEL, 0);
}

void rf212_int_handler()
{
	static int fail = 0;
	static int corr = 0;
	if (IORD(RF212_IRQ_BASE_KERNEL, 0)) {
		irq_status = rf212_register(READ, 0xF, 0);
		if (rf212_register(READ, 0x06, 0) & 0x80) {
			uint8_t data[256];
			uint8_t len = rf212_rx(data);
/*
			printf("len=%d\n", len);
			int i;
			for (i = 0; i < len + 3; ++i)
				printf("%02x ", data[i]);
			printf("\n");
*/
			uint32_t num;
			memcpy((uint8_t*)&num, data + 9, 4);
			++corr;
			kprintf("rf212 recv! len=%d first=0x%x corr=%d fail=%d\n", len, data[0], corr, fail);
		} else {
			kprintf("RX_CRC_VALID failed!!!");
			++fail;
		}
	}
}

void rf212_init()
{
	kprintf("rf212_init!\n");
	IOWR(RF212_SLP_TR_BASE_KERNEL , 0, 0);
	rf212_register(WRITE, 0x2, 0x8);
	do {
		trx_status = rf212_register(READ, 0x1, 0);
	} while (trx_status != 0x08);
	uint8_t part_num = rf212_register(READ, 0x1C, 0);
	uint8_t version_num = rf212_register(READ, 0x1D, 0);
	assert(part_num == 0x07);
	assert(version_num == 0x01);

	uint8_t trx_ctrl_0 = rf212_register(READ, 0x3, 0);
	kprintf("trx_ctrl_0=%x\n", trx_ctrl_0);
	trx_ctrl_0 = 0;
	rf212_register(WRITE, 0x3, trx_ctrl_0);//clkm

	rf212_register(WRITE, 0x2D, 0x79);//CSMA_SEED_0
	rf212_register(WRITE, 0x2E, 0x27);//CSMA_SEED_1

	rf212_register(WRITE, 0x0C, 0xA4);//RX_SAFE_MODE  40kbps
	//rf212_register(WRITE, 0x0C, 0x2E);//RX_SAFE_MODE 1000kbps

	rf212_register(WRITE, 0x04, 0x22);//TRX_CTRL_1
	rf212_register(WRITE, 0x0E, 0x08);//IRQ_MASK

	/*
	uint8_t irq_status = rf212_register(READ, 0xF, 0);
	printf("irq_status=%x\n", irq_status);
	*/

	rf212_register(WRITE, 0x08, 0x21);//PHY_CC_CCA
	rf212_register(WRITE, 0x16, 0x33);//RF_CTRL_0
	rf212_register(WRITE, 0x05, 0x61);//PHY_TX_PWR

	//PLL_ON
	trx_status = rf212_register(READ, 0x01, 0);
	kprintf("before PLL_ON trx_status=%x\n", trx_status);
	rf212_register(WRITE, 0x02, 0x09);
	do {
		irq_status = rf212_register(READ, 0xF, 0);
	} while (!(irq_status & 0x1));
	kprintf("trx_status=%x\n", rf212_register(READ, 0x01, 0));

	//RX_ON
	rf212_register(WRITE, 0x02, 0x06);
	kprintf("rx_on: trx_status=%x\n", rf212_register(READ, 0x01, 0));
	alt_irq_enable(RF212_IRQ_IRQ);
	IOWR_ALTERA_AVALON_PIO_IRQ_MASK(RF212_IRQ_BASE_KERNEL, 1);
}

void rf212_send(uint8_t len, uint8_t *data)
{
	kprintf("rf212_send len=%d first=0x%x\n", len, data[0]);
	if (len > 125) {
		kprintf("rf212_send : len = %d must not be more than 125!!!\n");
		return;
	}
	rf212_register(WRITE, 0x02, 0x04);//FORCE_PLL_ON
	alt_irq_disable(RF212_IRQ_IRQ);
	do {
		trx_status = rf212_register(READ, 0x01, 0);
	} while (trx_status != 0x09);

	rf212_tx(data, len);
	IOWR(RF212_SLP_TR_BASE_KERNEL, 0, 1);
	do {
	} while (!IORD(RF212_IRQ_BASE_KERNEL, 0));
	IOWR(RF212_SLP_TR_BASE_KERNEL, 0, 0);
	rf212_register(WRITE, 0x02, 0x06);//FORCE_PLL_ON
	do {
		trx_status = rf212_register(READ, 0x01, 0);
	} while (trx_status != 0x06);
	
	irq_status = rf212_register(READ, 0xF, 0);	
	alt_irq_enable(RF212_IRQ_IRQ);
	
	usleep(100000);
}


