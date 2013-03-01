/*
 * =====================================================================================
 *
 *       Filename:  nand_mtd.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/06/2012 07:59:08 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifndef __ARCH_ARM_DRIVER_MTD_H
#define __ARCH_ARM_DRIVER_MTD_H

#define MTD_ABSENT		0
#define MTD_RAM			1
#define MTD_ROM			2
#define MTD_NORFLASH		3
#define MTD_NANDFLASH		4
#define MTD_DATAFLASH		6
#define MTD_UBIVOLUME		7

struct nand_oobfree {
	uint32_t offset;
	uint32_t length;
};

#define MTD_MAX_OOBFREE_ENTRIES	8
/*
 * ECC layout control structure. Exported to userspace for
 * diagnosis and to allow creation of raw images
 */
struct nand_ecclayout {
	uint32_t eccbytes;
	uint32_t eccpos[64];
	uint32_t oobavail;
	struct nand_oobfree oobfree[MTD_MAX_OOBFREE_ENTRIES];
};

struct nand_chip {
	void (*write_cmd) (unsigned char cmd);
	void (*write_addr) (unsigned char addr);
	void (*enable_chip) (int enable);
	unsigned char (*read_byte) ();
	void (*write_byte) (unsigned char byte);
	void (*wait_rdy) ();
	void (*write_full_addr) (unsigned int row, unsigned int col);

	int (*ecc_calculate) (const unsigned char *dat, size_t sz,
			      unsigned char *ecc_code);
	int (*ecc_correct) (unsigned char *dat, unsigned char *read_ecc,
			    unsigned char *isnull);

	/* size of data buffer>=pg_size, size of spare buffer>=spare_size */
	int (*read_page) (struct nand_chip * chip, unsigned page_id,
			  unsigned char *data, unsigned char *spare,
			  int *eccStatus);

	/* spare data only contain user data, ecc auto-appended */
	int (*write_page) (struct nand_chip * chip, unsigned pageId,
			   const unsigned char *data, unsigned dataLength,
			   const unsigned char *spare, unsigned spareLength);

	int (*erase_block) (struct nand_chip * chip, unsigned blkID);
	/* return 0 if bad */
	int (*check_block) (struct nand_chip * chip, unsigned blkID);
	/* mark bad blk */
	int (*mark_badblock) (struct nand_chip * chip, unsigned blkID);

	struct nand_ecclayout *ecclayout;

	int badblock_offset;
	unsigned spare_size;
	unsigned blk_cnt;
	unsigned pages_per_blk;
	unsigned blk_shift;
	unsigned pg_size;
	unsigned pg_size_shift;
};

struct mtd_partition {
	char name[32];
	unsigned start_block;
	unsigned end_block;
};

/* kernel only */

/* Nand flash commands */
#define CMD_READ_1			0x00
#define CMD_READ_2			0x30

#define CMD_READID			0x90

#define CMD_WRITE_1			0x80
#define CMD_WRITE_2			0x10

#define CMD_ERASE_1			0x60
#define CMD_ERASE_2			0xD0

#define CMD_STATUS			0x70
#define CMD_RESET			0xFF

/* Nand flash commands (small blocks) */
#define CMD_READ_A0			0x00
#define CMD_READ_A1			0x01
#define CMD_READ_C			0x50

#define CMD_WRITE_A			0x00
#define CMD_WRITE_C			0x50

#define CMD_RANDOM_READ_1 0x05
#define CMD_RANDOM_READ_2 0xE0

#define NAND_BUS_WIDTH_8BITS		0x0
#define NAND_BUS_WIDTH_16BITS		0x1

extern int check_nandflash();
extern struct nand_chip *get_nand_chip();
extern struct mtd_partition *get_mtd_partition();
extern int get_mtd_partition_cnt();

#define MAX_SPARE_BUF 128
#define MAX_PAGE_BUF  4096

#endif
