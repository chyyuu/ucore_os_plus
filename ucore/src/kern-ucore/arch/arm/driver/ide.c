#include <types.h>
#include <stdio.h>
#include <kio.h>
#include <trap.h>
#include <picirq.h>
#include <fs.h>
#include <ide.h>
#include <arm.h>
#include <assert.h>
#include <board.h>
#include <nand_mtd.h>
#include <ramdisk.h>
#include <memlayout.h>

#define VALID_IDE(ideno)        (((ideno) >= 0) && ((ideno) < MAX_IDE) && (ide_devices[ideno] && ide_devices[ideno]->valid))

#define MAX_IDE 8

#define CHECK_CALL(ideno, func) (VALID_IDE(ideno) && ide_devices[ideno]->func)

//#define _CHECK_NANDFLASH

static struct ide_device *ide_devices[MAX_IDE];

static void check_nandflash_blk()
{
#ifdef HAS_NANDFLASH
	if (check_nandflash()) {
		kprintf("TEST NAND\n");
		unsigned char sp_buf[64];
		int j;
		unsigned char *buf = (unsigned char *)kmalloc(2048);
		assert(buf);
		struct nand_chip *chip = get_nand_chip();
		assert(chip);

#if 0
		chip->write_cmd(CMD_WRITE_1);
		chip->write_full_addr(0x40, 0);	//write main
		for (j = 0; j < 2048; j++) {
			chip->write_byte(0xab);
		}
		chip->write_cmd(CMD_WRITE_2);
		chip->wait_rdy();
		chip->wait_rdy();
		chip->write_cmd(CMD_STATUS);
		unsigned char status = chip->read_byte();
		kprintf("write %02x\n", status);
#endif
		//chip->erase_block(chip, 1);
		for (j = 0; j < 8; j++)
			sp_buf[j] = j;
		for (j = 0; j < 2048; j++)
			buf[j] = j;
		buf[0] = 0x12;
		//chip->write_page(chip, 0x40, buf, 2048, sp_buf, 8);

		int eccStatus;
		chip->read_page(chip, 0x40, buf, sp_buf, &eccStatus);
		for (j = 0; j < 16; j++)
			kprintf("%02x ", buf[j]);
		kprintf("\n");
		for (j = 0; j < 16; j++)
			kprintf("%02x ", sp_buf[j]);
		kprintf("\n");

		kprintf("ecc: %d\n", eccStatus);
		for (j = 0; j < chip->blk_cnt; j++) {
			if (!chip->check_block(chip, j))
				kprintf("BAD %08x\n", j);
		}
		kprintf("TEST NAND DONE\n");
	} else {
		kprintf("No NAND flash found!!!\n");
	}
#endif

}

static int ide_wait_ready(unsigned short iobase, bool check_error)
{
	//~ int r;
	//~ while ((r = inb(iobase + ISA_STATUS)) & IDE_BSY)
	//~ /* nothing */;
	//~ if (check_error && (r & (IDE_DF | IDE_ERR)) != 0) {
	//~ return -1;
	//~ }
	return 0;
}

// TODO: check if ramdisk are there
void ide_init(void)
{
	int i;
#ifdef UCONFIG_HAVE_RAMDISK
	int devno = DISK0_DEV_NO;
	assert(devno < MAX_IDE);
	ide_devices[devno] = (struct ide_device *)kmalloc(sizeof(struct ide_device));
	assert(ide_devices[devno] != NULL);
	ramdisk_init_struct(ide_devices[devno]);
	if (CHECK_CALL(devno, init))
		ide_devices[devno]->init(ide_devices[devno]);
#endif
#ifdef _CHECK_NANDFLASH
	check_nandflash_blk();
#endif

}

int ide_device_valid(unsigned short ideno)
{
	return VALID_IDE(ideno);
}

unsigned long ide_device_size(unsigned short ideno)
{
	if (ide_device_valid(ideno)) {
		return ide_devices[ideno]->lba;
	}
	return 0;
}

int
ide_read_secs(unsigned short ideno, unsigned long secno, void *dst,
	      unsigned long nsecs)
{
	if (CHECK_CALL(ideno, read_secs))
		return ide_devices[ideno]->read_secs(ide_devices[ideno], secno,
						     dst, nsecs);
	return 0;
}

int
ide_write_secs(unsigned short ideno, unsigned long secno, const void *src,
	       unsigned long nsecs)
{
	if (CHECK_CALL(ideno, write_secs))
		return ide_devices[ideno]->write_secs(ide_devices[ideno], secno,
						      src, nsecs);
	return 0;
}

int ide_register_device(unsigned short ideno, struct ide_device *dev)
{
	assert(dev != NULL);
	if (ide_devices[ideno]) {
		kprintf("ide_register_device: dev %d exists\n", ideno);
		return -1;
	}
	ide_devices[ideno] = dev;
	return 0;
}
