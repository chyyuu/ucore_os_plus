#include <ide.h>
#include <spi.h>
#include <sem.h>
#include <fs.h>
#include <board.h>
#include <assert.h>
#include <stdio.h>
#include <kio.h>
#include <string.h>

#define MAX_IDE         4

static struct ide_device {
	unsigned char valid;
	unsigned int base;	// base address in byte
	unsigned int size;	// size in sector/block
	unsigned char model[41];	// Model in string
	semaphore_t sem;
} ide_devices[MAX_IDE];

static void lock_device(unsigned short ideno)
{
	down(&(ide_devices[ideno].sem));
}

static void unlock_device(unsigned short ideno)
{
	up(&(ide_devices[ideno].sem));
}

void ide_init(void)
{
	int i;
	for (i = 0; i < MAX_IDE; i++)
		ide_devices[i].valid = 0;

#ifdef BOARD			/* Use SPI-sdcard */
	char buffer[BLOCK_SIZE];

	/* No matter whether the card has been used, we want to init it in our own way.
	 * So reset the sdcard controller
	 */
	REG8(SPI_BASE + SPI_MASTER_CONTROL_REG) = SPI_RST;

	/* Now inititialize it. */
	REG8(SPI_BASE + SPI_TRANS_TYPE_REG) = SPI_INIT_SD;
	REG8(SPI_BASE + SPI_TRANS_CTRL_REG) = SPI_TRANS_START;
	while (REG8(SPI_BASE + SPI_TRANS_STS_REG) == SPI_TRANS_BUSY) ;
	if ((REG8(SPI_BASE + SPI_TRANS_ERROR_REG) & SPI_SD_INIT_ERROR) !=
	    SPI_NO_ERROR)
		panic("Mass storage device not available.\n");

	/* Read the first block which contains the partition table. */
	REG8(SPI_BASE + SPI_SD_ADDR_7_0_REG) = 0;
	REG8(SPI_BASE + SPI_SD_ADDR_15_8_REG) = 0;
	REG8(SPI_BASE + SPI_SD_ADDR_23_16_REG) = 0;
	REG8(SPI_BASE + SPI_SD_ADDR_31_24_REG) = 0;
	REG8(SPI_BASE + SPI_TRANS_TYPE_REG) = SPI_RW_READ_SD_BLOCK;
	REG8(SPI_BASE + SPI_TRANS_CTRL_REG) = SPI_TRANS_START;
	while (REG8(SPI_BASE + SPI_TRANS_STS_REG) == SPI_TRANS_BUSY) ;
	if ((REG8(SPI_BASE + SPI_TRANS_ERROR_REG) & SPI_SD_READ_ERROR) !=
	    SPI_NO_ERROR)
		panic("Cannot read the first block of the device.\n");
	for (i = 0; i < BLOCK_SIZE; i++)
		buffer[i] = REG8(SPI_BASE + SPI_RX_FIFO_DATA_REG);

	/* Locate the table.
	 * The magic number '2' is the sign of the block at the end.
	 */
	struct partition_info_entry *entries = (struct partition_info_entry *)
	    ((uintptr_t) buffer + BLOCK_SIZE - 2 -
	     MAX_PARTITION_NUM * sizeof(struct partition_info_entry));

	/* The partitions should be used like:
	 *    0: kernel (ucore itself)
	 *    1: sfs
	 *    2: swap
	 *    3: (reserved)
	 */
	for (i = 0; i < MAX_PARTITION_NUM; i++, entries++) {
		if (!(entries->flag & PARTITION_ENTRY_FLAG_VALID))
			continue;

		ide_devices[i].valid = 1;
		ide_devices[i].base = entries->start * BLOCK_SIZE;
		ide_devices[i].size = entries->size;
		strcpy(ide_devices[i].model, "SPI-SDCARD");
		sem_init(&(ide_devices[i].sem), 1);
	}

#else /* RAM fs for simulation */
	/* Init disk fs */
	ide_devices[DISK0_DEV_NO].valid = 1;
	ide_devices[DISK0_DEV_NO].base = DISK_BASE + DISK_FS_BASE;
	ide_devices[DISK0_DEV_NO].size = DISK_FS_SIZE / SECTSIZE;
	strcpy((char *)ide_devices[DISK0_DEV_NO].model, "RAM fs");
	sem_init(&(ide_devices[DISK0_DEV_NO].sem), 1);

	/* Init swap fs */
	ide_devices[SWAP_DEV_NO].valid = 1;
	ide_devices[SWAP_DEV_NO].base = DISK_BASE + SWAP_FS_BASE;
	ide_devices[SWAP_DEV_NO].size = SWAP_FS_SIZE / SECTSIZE;
	strcpy((char *)ide_devices[SWAP_DEV_NO].model, "RAM swap fs");
	sem_init(&(ide_devices[SWAP_DEV_NO].sem), 1);
#endif

	/* Print info about our disks */
	for (i = 0; i < MAX_IDE; i++) {
		if (ide_devices[i].valid)
			kprintf("ide: %d: %10u(sectors), '%s'.\n", i,
				ide_devices[i].size, ide_devices[i].model);
	}
}

bool ide_device_valid(unsigned short ideno)
{
	return (ideno < MAX_IDE) ? ide_devices[ideno].valid : 0;
}

size_t ide_device_size(unsigned short ideno)
{
	return (ideno < MAX_IDE
		&& ide_devices[ideno].valid) ? ide_devices[ideno].size : 0;
}

int ide_read_secs(unsigned short ideno, uint32_t secno, void *dst, size_t nsecs)
{
	assert(ide_devices[ideno].valid);
	assert(secno + nsecs < ide_devices[ideno].size);

	int ret = 0;

	lock_device(ideno);

#ifdef BOARD
	char *buf = (char *)dst;
	uintptr_t address = ide_devices[ideno].base + secno * BLOCK_SIZE;
	int i, j;

	for (i = 0; i < nsecs; i++, address += BLOCK_SIZE, buf += BLOCK_SIZE) {
		REG8(SPI_BASE + SPI_SD_ADDR_7_0_REG) = address & 0xff;
		REG8(SPI_BASE + SPI_SD_ADDR_15_8_REG) = (address >> 8) & 0xff;
		REG8(SPI_BASE + SPI_SD_ADDR_23_16_REG) = (address >> 16) & 0xff;
		REG8(SPI_BASE + SPI_SD_ADDR_31_24_REG) = (address >> 24) & 0xff;
		REG8(SPI_BASE + SPI_TRANS_TYPE_REG) = SPI_RW_READ_SD_BLOCK;
		REG8(SPI_BASE + SPI_TRANS_CTRL_REG) = SPI_TRANS_START;
		while (REG8(SPI_BASE + SPI_TRANS_STS_REG) == SPI_TRANS_BUSY) ;
		if ((REG8(SPI_BASE + SPI_TRANS_ERROR_REG) & SPI_SD_READ_ERROR)
		    != SPI_NO_ERROR) {
			ret = -1;
			break;
		}
		for (j = 0; j < BLOCK_SIZE; j++)
			buf[j] = REG8(SPI_BASE + SPI_RX_FIFO_DATA_REG);
	}
#else
	memcpy(dst, (void *)(ide_devices[ideno].base + secno * SECTSIZE),
	       nsecs * SECTSIZE);
#endif

	unlock_device(ideno);
	return ret;
}

int
ide_write_secs(unsigned short ideno, uint32_t secno, const void *src,
	       size_t nsecs)
{
	assert(ide_devices[ideno].valid);
	assert(secno + nsecs < ide_devices[ideno].size);

	int ret = 0;

	lock_device(ideno);

#ifdef BOARD
	char *buf = (char *)src;
	uintptr_t address = ide_devices[ideno].base + secno * BLOCK_SIZE;
	int i, j;

	for (i = 0; i < nsecs; i++, address += BLOCK_SIZE, buf += BLOCK_SIZE) {
		for (j = 0; j < BLOCK_SIZE; j++)
			REG8(SPI_BASE + SPI_TX_FIFO_DATA_REG) = buf[j];
		REG8(SPI_BASE + SPI_SD_ADDR_7_0_REG) = address & 0xff;
		REG8(SPI_BASE + SPI_SD_ADDR_15_8_REG) = (address >> 8) & 0xff;
		REG8(SPI_BASE + SPI_SD_ADDR_23_16_REG) = (address >> 16) & 0xff;
		REG8(SPI_BASE + SPI_SD_ADDR_31_24_REG) = (address >> 24) & 0xff;
		REG8(SPI_BASE + SPI_TRANS_TYPE_REG) = SPI_RW_WRITE_SD_BLOCK;
		REG8(SPI_BASE + SPI_TRANS_CTRL_REG) = SPI_TRANS_START;
		while (REG8(SPI_BASE + SPI_TRANS_STS_REG) == SPI_TRANS_BUSY) ;
		if ((REG8(SPI_BASE + SPI_TRANS_ERROR_REG) & SPI_SD_WRITE_ERROR)
		    != SPI_NO_ERROR) {
			ret = -1;
			break;
		}
	}
#else
	memcpy((void *)(ide_devices[ideno].base + secno * SECTSIZE), src,
	       nsecs * SECTSIZE);
#endif

	unlock_device(ideno);
	return ret;
}
