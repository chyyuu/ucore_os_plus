/*
 * =====================================================================================
 *
 *       Filename:  at91-nandflash.c
 *
 *    Description:  at91 smc nandflash interface support
 *
 *        Version:  1.0
 *        Created:  04/07/2012 12:30:44 PM
 *       Revision:  none
 *       Compiler:  gcc 4.4
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#include <stdlib.h>

#include <arm.h>
#include <board.h>
#include <at91-gpio.h>
#include <at91-nandflash.h>
#include <nand_mtd.h>
#include <at91cap9_matrix.h>
#include <at91-pmc.h>
#include <kio.h>
#include <memlayout.h>
#include <assert.h>

#ifdef HAS_NANDFLASH

static int nandflash_init = 0;

/*----------------------------------------------------------------------------*/
/* NAND Commands							      */
/*----------------------------------------------------------------------------*/
/* 8 bits devices */
#define WRITE_NAND_COMMAND(d) do { \
	*(volatile unsigned char *) \
	((unsigned long)AT91C_SMARTMEDIA_BASE | AT91_SMART_MEDIA_CLE) = \
	(unsigned char)(d); \
	} while(0)

#define WRITE_NAND_ADDRESS(d) do { \
	*(volatile unsigned char *) \
	((unsigned long)AT91C_SMARTMEDIA_BASE | AT91_SMART_MEDIA_ALE) = \
	(unsigned char)(d); \
	} while(0)

#define WRITE_NAND(d) do { \
	*(volatile unsigned char *) \
	((unsigned long)AT91C_SMARTMEDIA_BASE) = (unsigned char)d; \
	} while(0)

#define READ_NAND() ((unsigned char)(*(volatile unsigned char *) \
	(unsigned long)AT91C_SMARTMEDIA_BASE))

/* 16 bits devices */
#define WRITE_NAND_COMMAND16(d) do { \
	*(volatile unsigned short *) \
	((unsigned long)AT91C_SMARTMEDIA_BASE | AT91_SMART_MEDIA_CLE) = \
	(unsigned short)(d); \
	} while(0)

#define WRITE_NAND_ADDRESS16(d) do { \
	*(volatile unsigned short *) \
	((unsigned long)AT91C_SMARTMEDIA_BASE | AT91_SMART_MEDIA_ALE) = \
	(unsigned short)(d); \
	} while(0)

#define WRITE_NAND16(d) do { \
	*(volatile unsigned short *) \
	((unsigned long)AT91C_SMARTMEDIA_BASE) = (unsigned short)d; \
	} while(0)

#define READ_NAND16() ((unsigned short)(*(volatile unsigned short *) \
						(unsigned long)AT91C_SMARTMEDIA_BASE))

#define NAND_DISABLE_CE() do { *(volatile unsigned int *)(AT91C_BASE_PIOC+PIO_SODR)= ek_nand_config.enable_pin_mask;} while(0)
#define NAND_ENABLE_CE()  do { *(volatile unsigned int *)(AT91C_BASE_PIOC+PIO_CODR) = ek_nand_config.enable_pin_mask;} while(0)

#define NAND_WAIT_READY() while (!(*(volatile unsigned int *)(AT91C_BASE_PIOC+PIO_PDSR)& ek_nand_config.rdy_pin_mask))

/* Register access macros */
#define ecc_readl(add, reg)				\
	inw((add) + ATMEL_ECC_##reg)
#define ecc_writel(add, reg, value)			\
	outw( (add) + ATMEL_ECC_##reg, value)

#include "atmel_nand_ecc.h"	/* Hardware ECC registers */

static inline void pio_cfg_input(uintptr_t base, unsigned int pin,
				 int use_pullup)
{
	outw(base + PIO_IDR, pin);
	outw(base + (use_pullup ? PIO_PPUER : PIO_PPUDR), pin);
	outw(base + PIO_ODR, pin);
	outw(base + PIO_PER, pin);
}

static inline void pio_cfg_output(uintptr_t base, unsigned int pin, int value)
{
	outw(base + PIO_IDR, pin);
	outw(base + PIO_PPUDR, pin);
	outw(base + (value ? PIO_SODR : PIO_CODR), pin);
	outw(base + PIO_OER, pin);
	outw(base + PIO_PER, pin);
}

static inline void pio_cfg_A_periph(uintptr_t base, unsigned int pin,
				    int use_pullup)
{
	outw(base + PIO_IDR, pin);
	outw(base + (use_pullup ? PIO_PPUER : PIO_PPUDR), pin);
	outw(base + PIO_ASR, pin);
	outw(base + PIO_PDR, pin);
}

static struct sam9_nand_config ek_nand_config = {
	.smc_setup = 0x00020002,
	.smc_pulse = 0x04040404,
	.smc_cycle = 0x00070007,
	.smc_mode =
	    (3 << 16) | AT91C_SMC_READMODE | AT91C_SMC_WRITEMODE |
	    AT91C_SMC_NWAITM_NWAIT_DISABLE,

	.ale = 21,
	.cle = 22,

	.rdy_pin_mask = AT91C_PIO_PC8,
	.enable_pin_mask = AT91C_PIO_PC14,

	.bus_width_16 = 0,

	.io_addr_base = NAND_FS_VBASE,
	.io_addr_phy_base = AT91C_SMARTMEDIA_BASE,

	.io_addr_ecc_phy_base = AT91C_BASE_ECC,
	.io_addr_ecc_base = AT91C_BASE_ECC,

	.info = NULL,
};

static struct nand_chip chip;

static void atmel_nand_write_cmd(unsigned char cmd)
{
	*(volatile unsigned char *)(ek_nand_config.io_addr_base
				    | AT91_SMART_MEDIA_CLE) = cmd;
}

static void atmel_nand_write_addr(unsigned char addr)
{
	*(volatile unsigned char *)(ek_nand_config.io_addr_base
				    | AT91_SMART_MEDIA_ALE) = addr;
}

static unsigned char atmel_nand_read_byte()
{
	return *(volatile unsigned char *)(ek_nand_config.io_addr_base);
}

static void atmel_nand_write_byte(unsigned char byte)
{
	*(volatile unsigned char *)(ek_nand_config.io_addr_base) = byte;
}

static void atmel_wait_rdy()
{
	while (!(*(volatile unsigned int *)(AT91C_BASE_PIOC + PIO_PDSR)
		 & ek_nand_config.rdy_pin_mask)) ;
}

static void atmel_enable_chip(int enable)
{
	if (enable) {
		NAND_ENABLE_CE();
	} else {
		NAND_DISABLE_CE();
	}
}

static void atmel_write_full_addr(unsigned int row, unsigned int col)
{
	atmel_nand_write_addr(col & 0xff);
	atmel_nand_write_addr(col >> 8);
	atmel_nand_write_addr(row & 0xff);
	atmel_nand_write_addr((row >> 8) & 0xff);
	atmel_nand_write_addr(row >> 16);
}

static int atmel_read_page(struct nand_chip *chip, unsigned page_id,
			   unsigned char *data,
			   unsigned char *spare, int *eccStatus)
{
	int j;
	chip->enable_chip(1);
	chip->write_cmd(CMD_READ_1);
	chip->write_full_addr(page_id, 0);	//read main
	chip->write_cmd(CMD_READ_2);
	chip->wait_rdy();
	chip->wait_rdy();
	for (j = 0; j < chip->pg_size; j++) {
		data[j] = chip->read_byte();
	}
	/* read ECC */
	uint32_t *eccpos = chip->ecclayout->eccpos;
	if (eccpos[0] != 0) {
		//jump to ecc
		chip->write_cmd(CMD_RANDOM_READ_1);
		chip->write_addr((chip->pg_size + eccpos[0]) & 0xff);
		chip->write_addr((chip->pg_size + eccpos[0]) >> 8);
		chip->write_cmd(CMD_RANDOM_READ_2);
	}
	for (j = 0; j < chip->ecclayout->eccbytes; j++) {
		spare[j] = chip->read_byte();
		//kprintf("%02x ", spare[j]);
	}
	//kprintf(" @@\n");

	*eccStatus = chip->ecc_correct(data, spare, NULL);

	chip->write_cmd(CMD_RANDOM_READ_1);
	chip->write_addr(chip->pg_size & 0xff);
	chip->write_addr(chip->pg_size >> 8);
	chip->write_cmd(CMD_RANDOM_READ_2);
	for (j = 0; j < chip->spare_size; j++)
		spare[j] = chip->read_byte();
	chip->enable_chip(0);

	return 0;
}

static int atmel_write_page(struct nand_chip *chip, unsigned pageId,
			    const unsigned char *data, unsigned dataLength,
			    const unsigned char *spare, unsigned spareLength)
{
	unsigned char spare_buf[MAX_SPARE_BUF];
	int j;
	assert(dataLength <= chip->pg_size);
	assert(spareLength <= chip->ecclayout->oobfree[0].length);
	memset(spare_buf, 0xff, MAX_SPARE_BUF);
	memcpy(spare_buf + chip->ecclayout->oobfree[0].offset, spare,
	       spareLength);

	//spare_buf[chip->badblock_offset] = 0xff;

	chip->enable_chip(1);
	chip->write_cmd(CMD_WRITE_1);
	chip->write_full_addr(pageId, 0);	//write main
	for (j = 0; j < dataLength; j++) {
		chip->write_byte(data[j]);
	}
	for (; j < chip->pg_size; j++)
		chip->write_byte(0xFF);

	chip->ecc_calculate(NULL, 0, spare_buf + chip->ecclayout->eccpos[0]);

	for (j = 0; j < chip->spare_size; j++)
		chip->write_byte(spare_buf[j]);

	chip->write_cmd(CMD_WRITE_2);

	chip->wait_rdy();
	chip->wait_rdy();

#if 0
	chip->write_cmd(CMD_STATUS);
	unsigned char status = chip->read_byte();
	kprintf("write %02x\n", status);
#endif
	chip->enable_chip(0);

	return 0;

}

static int atmel_erase_block(struct nand_chip *chip, unsigned blkID)
{
	unsigned long addr = blkID * chip->pages_per_blk;
	//kprintf("##ERASE NAND: %08x\n", addr);
	chip->enable_chip(1);
	chip->write_cmd(CMD_ERASE_1);
	chip->write_addr(addr & 0xff);
	chip->write_addr((addr >> 8) & 0xff);
	chip->write_addr(addr >> 16);
	chip->write_cmd(CMD_ERASE_2);
	chip->wait_rdy();
	chip->wait_rdy();
	chip->enable_chip(0);
	return 0;
}

static int atmel_check_block(struct nand_chip *chip, unsigned blkID)
{
	chip->enable_chip(1);
	chip->write_cmd(CMD_READ_1);
	chip->write_full_addr(blkID * chip->pages_per_blk, 0);	//read main
	chip->write_cmd(CMD_READ_2);
	chip->wait_rdy();
	chip->wait_rdy();
	chip->write_cmd(CMD_RANDOM_READ_1);
	chip->write_addr((chip->pg_size + chip->badblock_offset) & 0xff);
	chip->write_addr((chip->pg_size + chip->badblock_offset) >> 8);
	chip->write_cmd(CMD_RANDOM_READ_2);
	unsigned char state = chip->read_byte();
	chip->enable_chip(0);
	return (state == 0xff);

}

static int atmel_mark_badblock(struct nand_chip *chip, unsigned blkID)
{
	int j;
	//panic("DANGER\n");
	chip->enable_chip(1);
	chip->write_cmd(CMD_WRITE_1);
	chip->write_full_addr(blkID * chip->pages_per_blk, 0);	//write main
	for (j = 0; j < chip->pg_size; j++)
		chip->write_byte(0xFF);
	//mark
	for (j = 0; j < chip->spare_size; j++)
		chip->write_byte(0x00);

	chip->write_cmd(CMD_WRITE_2);

	chip->wait_rdy();
	chip->wait_rdy();
	chip->enable_chip(0);

	return 0;
}

/* from Linux 2.6 */
/*
 * Calculate HW ECC
 *
 * function called after a write
 *
 * dat:        raw data (unused)
 * ecc_code:   buffer for ECC
 */
static int atmel_nand_calculate(const unsigned char *dat, size_t sz,
				unsigned char *ecc_code)
{
	unsigned int ecc_value;

	/* get the first 2 ECC bytes */
	ecc_value = ecc_readl(ek_nand_config.io_addr_ecc_base, PR);

	ecc_code[0] = ecc_value & 0xFF;
	ecc_code[1] = (ecc_value >> 8) & 0xFF;

	/* get the last 2 ECC bytes */
	ecc_value =
	    ecc_readl(ek_nand_config.io_addr_ecc_base, NPR) & ATMEL_ECC_NPARITY;

	ecc_code[2] = ecc_value & 0xFF;
	ecc_code[3] = (ecc_value >> 8) & 0xFF;

#if 0
	int j;
	for (j = 0; j < 15; j++)
		kprintf("%08x \n", inw(AT91C_BASE_ECC + j * 4));
	kprintf("**************\n");
#endif
	return 0;
}

/* from Linux 2.6 */
/*
 * HW ECC Correction
 *
 * function called after a read
 *
 * mtd:        MTD block structure
 * dat:        raw data read from the chip
 * read_ecc:   ECC from the chip (unused)
 * isnull:     unused
 *
 * Detect and correct a 1 bit error for a page
 */
static int atmel_nand_correct(unsigned char *dat,
			      unsigned char *read_ecc, unsigned char *isnull)
{
	unsigned int ecc_status;
	unsigned int ecc_word, ecc_bit;

	/* get the status from the Status Register */
	ecc_status = ecc_readl(ek_nand_config.io_addr_ecc_base, SR);

	/* if there's no error */
	if (!(ecc_status & ATMEL_ECC_RECERR))
		return 0;

	/* get error bit offset (4 bits) */
	ecc_bit =
	    ecc_readl(ek_nand_config.io_addr_ecc_base, PR) & ATMEL_ECC_BITADDR;
	/* get word address (12 bits) */
	ecc_word =
	    ecc_readl(ek_nand_config.io_addr_ecc_base, PR) & ATMEL_ECC_WORDADDR;
	ecc_word >>= 4;

	/* if there are multiple errors */
	if (ecc_status & ATMEL_ECC_MULERR) {
		/* check if it is a freshly erased block
		 * (filled with 0xff) */
		if ((ecc_bit == ATMEL_ECC_BITADDR)
		    && (ecc_word == (ATMEL_ECC_WORDADDR >> 4))) {
			/* the block has just been erased, return OK */
			return 0;
		}
		/* it doesn't seems to be a freshly
		 * erased block.
		 * We can't correct so many errors */
		kprintf("atmel_nand : multiple errors detected."
			" Unable to correct.\n");
		return -1;
	}

	/* if there's a single bit error : we can correct it */
	if (ecc_status & ATMEL_ECC_ECCERR) {
		/* there's nothing much to do here.
		 * the bit error is on the ECC itself.
		 */
		kprintf("atmel_nand : one bit error on ECC code."
			" Nothing to correct\n");
		return 1;
	}

	kprintf("atmel_nand : one bit error on data."
		" (word offset in the page :"
		" 0x%x bit offset : 0x%x)\n", ecc_word, ecc_bit);
	/* correct the error */
	if (ek_nand_config.bus_width_16) {
		/* 16 bits words */
		((unsigned short *)dat)[ecc_word] ^= (1 << ecc_bit);
	} else {
		/* 8 bits words */
		dat[ecc_word] ^= (1 << ecc_bit);
	}
	kprintf("atmel_nand : error corrected\n");
	return 1;
}

static struct SNandInitInfo NandFlash_InitInfo[] = {
	/*
	 * ID    blk    blk_size pg_size_shift spare_size bus_width spare_scheme chip_name 
	 */
	{0x2cca, 0x800, 0x20000, 11, 0x40, 0x1, NULL, "MT29F2G16AAB"},
	{0x2cda, 0x800, 0x20000, 11, 0x40, 0x0, NULL, "MT29F2G08AAC"},
	{0x2caa, 0x800, 0x20000, 11, 0x40, 0x0, NULL, "MT29F2G08ABD"},
	{0xecda, 0x800, 0x20000, 11, 0x40, 0x0, NULL, "K9F2G08U0M"},
	{0xecaa, 0x800, 0x20000, 11, 0x40, 0x0, NULL, "K9F2G08U0A"},
	{0x20aa, 0x800, 0x20000, 11, 0x40, 0x0, NULL, "ST NAND02GR3B"},
	{0,}
};

/* oob layout for large page size
 * bad block info is on bytes 0 and 1
 * the bytes have to be consecutives to avoid
 * several NAND_CMD_RNDOUT during read
 */
static struct nand_ecclayout atmel_oobinfo_large = {
	.eccbytes = 4,
	.eccpos = {60, 61, 62, 63},
	.oobfree = {
		    {2, 58}
		    },
};

static int nandflash_probe(unsigned short chipID)
{
	SNandInitInfo *nii = NandFlash_InitInfo;
	while (nii->uNandID) {
		if (nii->uNandID == chipID) {
			if (nii->uNandSpareSize != 0x40) {
				kprintf
				    ("only support 64byte spare area nand\n");
				break;
			}
			ek_nand_config.info = nii;
			READ_NAND();	//don't care
			unsigned char info = READ_NAND();
			kprintf
			    ("Nandflash Detected: chipID: 0x%04x, type: %s, byte3: 0x%02x\n",
			     chipID, nii->name, info);
			goto found;
		}
		nii++;
	}

	kprintf("Not supported Nandflash: chipID: 0x%04x\n", chipID);
	return 0;

found:
	if (!ek_nand_config.bus_width_16) {	//8-bit   
		chip.write_cmd = atmel_nand_write_cmd;
		chip.write_addr = atmel_nand_write_addr;
		chip.read_byte = atmel_nand_read_byte;
		chip.write_byte = atmel_nand_write_byte;
		chip.write_full_addr = atmel_write_full_addr;
		chip.ecc_calculate = atmel_nand_calculate;
		chip.ecc_correct = atmel_nand_correct;
		chip.read_page = atmel_read_page;
		chip.write_page = atmel_write_page;
		chip.erase_block = atmel_erase_block;
		chip.check_block = atmel_check_block;
		chip.mark_badblock = atmel_mark_badblock;
		chip.badblock_offset = 0x00;
	} else {
		kprintf("NOT impl");
	}
	chip.wait_rdy = atmel_wait_rdy;
	chip.enable_chip = atmel_enable_chip;
	chip.spare_size = nii->uNandSpareSize;
	chip.pg_size_shift = nii->uNandSectorSize;
	chip.pg_size = 1 << (unsigned long)nii->uNandSectorSize;
	chip.blk_cnt = nii->uNandNbBlocks;
	chip.blk_shift = chip.pg_size_shift + 1;
	chip.pages_per_blk = nii->uNandBlockSize / chip.pg_size;

	ecc_writel(ek_nand_config.io_addr_ecc_phy_base, CR, ATMEL_ECC_RST);
	switch (chip.pg_size) {
	case 2048:
		chip.ecclayout = &atmel_oobinfo_large;
		ecc_writel(ek_nand_config.io_addr_ecc_phy_base, MR,
			   ATMEL_ECC_PAGESIZE_2112);
		break;
	default:
		kprintf("Unsupported nand pgsize\n");
		return 0;
	}
	kprintf("Nandflash size: %dMB, spare area: %dByte, pg_size: %dByte.\n",
		chip.pg_size * chip.pages_per_blk * chip.blk_cnt / 1024 / 1024,
		chip.spare_size, chip.pg_size);
	return 1;
}

/*------------------------------------------------------------------------------*/
/* \fn    nandflash_hw_init							*/
/* \brief NandFlash HW init							*/
/*------------------------------------------------------------------------------*/
int nandflash_hw_init(void)
{
	if (nandflash_init)
		return 0;

	unsigned long csa = inw(AT91C_BASE_MATRIX + AT91_MATRIX_EBICSA);
	outw(AT91C_BASE_MATRIX + AT91_MATRIX_EBICSA,
	     (csa | AT91_MATRIX_EBI_CS3A));
	csa = inw(AT91C_BASE_MATRIX + AT91_MATRIX_EBICSA);
	//Nandflash CS
	pio_cfg_output(AT91C_BASE_PIOC, ek_nand_config.enable_pin_mask, 1);

	// Configure Ready/Busy signal
	pio_cfg_input(AT91C_BASE_PIOC, ek_nand_config.rdy_pin_mask, 1);
	/* Enable PeriphClock */
	//kprintf("DD %08x\n", AT91C_BASE_PMC+PMC_PCSR);
	outw(AT91C_BASE_PMC + PMC_PCER, 1 << AT91C_ID_PIOC);
	//kprintf("DD %08x\n", AT91C_BASE_PMC+PMC_PCSR);

	// Configure SMC CS3
	outw(AT91C_BASE_SMC + SMC_SETUP3, ek_nand_config.smc_setup);
	outw(AT91C_BASE_SMC + SMC_PULSE3, ek_nand_config.smc_pulse);
	outw(AT91C_BASE_SMC + SMC_CYCLE3, ek_nand_config.smc_cycle);
	outw(AT91C_BASE_SMC + SMC_CTRL3,
	     ek_nand_config.smc_mode |
	     (ek_nand_config.bus_width_16 ? AT91C_SMC_DBW_WIDTH_SIXTEEN_BITS
	      : AT91C_SMC_DBW_WIDTH_EIGTH_BITS));

	/* try to read chipID */
	NAND_ENABLE_CE();
	//WRITE_NAND_COMMAND(CMD_RESET);
	/*
	 * Ask the Nand its IDs 
	 */
	WRITE_NAND_COMMAND(CMD_READID);
	WRITE_NAND_ADDRESS(0x00);
//    kprintf("# %08x\n", *(volatile unsigned*)(AT91C_BASE_PIOC+PIO_PDSR));
	/*
	 * Read answer 
	 */
	unsigned char bManufacturerID = READ_NAND();
	unsigned char bDeviceID = READ_NAND();
	unsigned short chipId =
	    (((unsigned short)bManufacturerID) << 8) | bDeviceID;

	nandflash_init = nandflash_probe(chipId);

	NAND_DISABLE_CE();

	return 0;

#if 0
	if (nandinfo) {
		nandflash_init = 1;
		kprintf("nandflash_hw_init() done, chipId=0x%04x!\n",
			nandinfo->uNandID);
	} else
		kprintf("nandflash_hw_init() failed.\n");
#endif

}

/*------------------------------------------------------------------------------*/
/* \fn    nandflash_cfg_16bits_dbw_init						*/
/* \brief Configure SMC in 16 bits mode						*/
/*------------------------------------------------------------------------------*/
void nandflash_cfg_16bits_dbw_init(void)
{
	outw(AT91C_BASE_SMC + SMC_CTRL3,
	     inw(AT91C_BASE_SMC +
		 SMC_CTRL3) | AT91C_SMC_DBW_WIDTH_SIXTEEN_BITS);
}

/*------------------------------------------------------------------------------*/
/* \fn    nandflash_cfg_8bits_dbw_init						*/
/* \brief Configure SMC in 8 bits mode						*/
/*------------------------------------------------------------------------------*/
void nandflash_cfg_8bits_dbw_init(void)
{
	outw(AT91C_BASE_SMC + SMC_CTRL3,
	     (inw(AT91C_BASE_SMC + SMC_CTRL3) & ~(AT91C_SMC_DBW)) |
	     AT91C_SMC_DBW_WIDTH_EIGTH_BITS);
}

int check_nandflash()
{
	return nandflash_init;
}

struct nand_chip *get_nand_chip()
{
	return check_nandflash()? (&chip) : NULL;
}

#else

int check_nandflash()
{
	return 0;
}

struct nand_chip *get_nand_chip()
{
	return NULL;
}
#endif /* HAS_NANDFLASH */
