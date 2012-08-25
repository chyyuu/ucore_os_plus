/*
 * =====================================================================================
 *
 *       Filename:  mtd_glue.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/09/2012 12:32:17 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#ifdef ARCH_ARM
#include <board.h>
#endif


#include "yaffs_nandif.h"
#include "yaffs_guts.h"
#include <nand_mtd.h>


/* mtd glue */
#define PARTITION_CNT 2
static struct yaffs_dev yaffsDev[PARTITION_CNT];

#if 0
static yaffsfs_DeviceConfiguration yaffsfs_config[] = {
	{ "/boot", &bootDev},
	{ "/data", &dataDev},
	{(void *)0,(void *)0}
};
#endif


static struct mtd_partition partition[] = {
  { "boot", 0, 9 },
  { "data", 10, 0x400}, //128M = 0x400*64*2K
};

struct mtd_partition* get_mtd_partition()
{
  return partition;
}


int    get_mtd_partition_cnt()
{
  return PARTITION_CNT;
}

static int mtd_initialise(struct yaffs_dev *dev)
{
  kprintf("mtd_initialise()\n");
  /* nothing to do here */
  if(!check_nandflash())
    panic("mtd_initialise: NAND not found!");
  return YAFFS_OK;
}

static int mtd_deinitialise(struct yaffs_dev *dev)
{
  /* TODO */
  return YAFFS_OK;
}

static unsigned char data_page_buf[MAX_PAGE_BUF];

static int mtd_read_page(struct yaffs_dev *dev,
					  unsigned pageId, 
					  unsigned char *data, unsigned dataLength,
					  unsigned char *spare, unsigned spareLength,
					  int *eccStatus)
{
  struct nand_chip *chip = (struct nand_chip*)(dev->os_context);
  unsigned char tmp_spare[MAX_SPARE_BUF];
  int ret = YAFFS_OK;

  if(dataLength>=chip->pg_size)
    ret = chip->read_page(chip, pageId, data, tmp_spare, eccStatus);
  else{
    ret = chip->read_page(chip, pageId, data_page_buf, tmp_spare, eccStatus);
    memcpy(data, data_page_buf, dataLength);
  }
  memcpy(spare, tmp_spare+chip->ecclayout->oobfree[0].offset,
      spareLength);
  ret = (ret==0)?YAFFS_OK:YAFFS_FAIL;

  return ret;
}

static int mtd_write_page(struct yaffs_dev *dev, unsigned pageId, 
					  const unsigned char *data, unsigned dataLength,
            const unsigned char *spare, unsigned spareLength)
{
  struct nand_chip *chip = (struct nand_chip*)(dev->os_context);
  int ret = chip->write_page(chip, pageId, data, dataLength, spare, spareLength);
  return (ret)==0?YAFFS_OK:YAFFS_FAIL;
}

static int mtd_erase_block(struct yaffs_dev *dev, unsigned blockId)
{
 struct nand_chip *chip = (struct nand_chip*)(dev->os_context);
  int ret = chip->erase_block(chip, blockId);
  return (ret==0)?YAFFS_OK:YAFFS_FAIL;
}

static int mtd_check_block(struct yaffs_dev *dev, unsigned blockId)
{
 struct nand_chip *chip = (struct nand_chip*)(dev->os_context);
  int ret = chip->check_block(chip, blockId);
  return ret?YAFFS_OK:YAFFS_FAIL;
}

static int mtd_mark_block(struct yaffs_dev *dev, unsigned blockId)
{
 struct nand_chip *chip = (struct nand_chip*)(dev->os_context);
  int ret = chip->mark_badblock(chip, blockId);
  return (ret==0)?YAFFS_OK:YAFFS_FAIL;
}

/* assume that only on nandfalsh */
static  ynandif_Geometry geo;

int yaffs_start_up(void)
{
  if(!check_nandflash()){
    kprintf("yaffs_start_up: no nandflash\n");
    return -1;
  }
	static int start_up_called = 0;

	if(start_up_called)
		return 0;
	start_up_called = 1;
  kprintf("yaffs_start_up()\n");

	// Stuff to configure YAFFS
	// Stuff to initialise anything special (eg lock semaphore).
	yaffsfs_OSInitialisation();

  int i = 0;
  struct nand_chip *chip = get_nand_chip();
  memset(&geo, 0, sizeof(ynandif_Geometry));

  geo.initialise = mtd_initialise;
  geo.deinitialise = mtd_deinitialise;
  geo.readChunk = mtd_read_page;
  geo.writeChunk = mtd_write_page;
  geo.eraseBlock = mtd_erase_block;
  geo.checkBlockOk = mtd_check_block;
  geo.markBlockBad = mtd_mark_block;

  geo.hasECC = 1; /* mtd ecc */
  geo.dataSize = chip->pg_size;
  geo.spareSize = chip->spare_size;
  geo.pagesPerBlock = chip->pages_per_blk;
  geo.inband_tags = 0;
  geo.useYaffs2 = 1;
  
  for(i=0;i<PARTITION_CNT;i++){
    geo.start_block = partition[i].start_block;
    geo.end_block = partition[i].end_block;
    yaffs_add_dev_from_geometry(&yaffsDev[i],
        partition[i].name, &geo);
    yaffsDev[i].os_context = (void*)chip;
  }

  return 0;
}

void mtd_erase_partition(struct nand_chip*chip ,const char* name)
{
  int i,j;
  assert(chip);
  for(i=0;i<PARTITION_CNT;i++){
    if(!strcmp(partition[i].name, name))
      goto found;
  }
  kprintf("mtd_erase_partition: '%s' not found\n", name);
  return ;
found:
  kprintf("mtd_erase_partition: erasing '%s'...\n", name);
  for(j=partition[i].start_block;j<=partition[i].end_block;j++){
    if(chip->check_block(chip,j))
      chip->erase_block(chip, j);
    else
      kprintf("mtd_erase_partition: warning, badblk 0x%08x\n", j);
  }
  kprintf("mtd_erase_partition: '%s' erased\n", name);
}


