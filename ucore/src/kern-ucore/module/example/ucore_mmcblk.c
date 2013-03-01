/*
 * =====================================================================================
 *
 *       Filename:  ucore_mmcblk.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/17/2012 01:28:03 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>

#include <linux/scatterlist.h>

//ucore
#include <ide.h>
#include "part.h"

#define RESULT_OK		0
#define RESULT_FAIL		1
#define RESULT_UNSUP_HOST	2
#define RESULT_UNSUP_CARD	3

#define BUFFER_ORDER		2
#define BUFFER_SIZE		(PAGE_SIZE << BUFFER_ORDER)

#define MAX_PARTITIONS 4
#define MAX_LABEL_LEN  31

//////////////////////////////

struct mmc_ucore_card {
	struct mmc_card *card;
	u8 *buffer;
	struct ide_device *ucore_dev;
	/* only use the first part */
	int partitioned;
	size_t part0_start_sec;
};

/*
 * Configure correct block size in card
 */
static int mmc_test_set_blksize(struct mmc_ucore_card *test, unsigned size)
{
	struct mmc_command cmd;
	int ret;

	cmd.opcode = MMC_SET_BLOCKLEN;
	cmd.arg = size;
	cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;
	ret = mmc_wait_for_cmd(test->card->host, &cmd, 0);
	if (ret)
		return ret;

	return 0;
}

/*
 * Fill in the mmc_request structure given a set of transfer parameters.
 */
static void mmc_test_prepare_mrq(struct mmc_ucore_card *test,
				 struct mmc_request *mrq,
				 struct scatterlist *sg, unsigned sg_len,
				 unsigned dev_addr, unsigned blocks,
				 unsigned blksz, int write)
{
	BUG_ON(!mrq || !mrq->cmd || !mrq->data || !mrq->stop);

	if (blocks > 1) {
		mrq->cmd->opcode = write ?
		    MMC_WRITE_MULTIPLE_BLOCK : MMC_READ_MULTIPLE_BLOCK;
	} else {
		mrq->cmd->opcode = write ?
		    MMC_WRITE_BLOCK : MMC_READ_SINGLE_BLOCK;
	}

	mrq->cmd->arg = dev_addr;
	mrq->cmd->flags = MMC_RSP_R1 | MMC_CMD_ADTC;

	if (blocks == 1)
		mrq->stop = NULL;
	else {
		mrq->stop->opcode = MMC_STOP_TRANSMISSION;
		mrq->stop->arg = 0;
		mrq->stop->flags = MMC_RSP_R1B | MMC_CMD_AC;
	}

	mrq->data->blksz = blksz;
	mrq->data->blocks = blocks;
	mrq->data->flags = write ? MMC_DATA_WRITE : MMC_DATA_READ;
	mrq->data->sg = sg;
	mrq->data->sg_len = sg_len;

	mmc_set_data_timeout(mrq->data, test->card);
}

/*
 * Wait for the card to finish the busy state
 */
static int mmc_test_wait_busy(struct mmc_ucore_card *test)
{
	int ret, busy;
	struct mmc_command cmd;

	busy = 0;
	do {
		memset(&cmd, 0, sizeof(struct mmc_command));

		cmd.opcode = MMC_SEND_STATUS;
		cmd.arg = test->card->rca << 16;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;

		ret = mmc_wait_for_cmd(test->card->host, &cmd, 0);
		if (ret)
			break;

		if (!busy && !(cmd.resp[0] & R1_READY_FOR_DATA)) {
			busy = 1;
			printk(KERN_INFO "%s: Warning: Host did not "
			       "wait for busy state to end.\n",
			       mmc_hostname(test->card->host));
		}
	} while (!(cmd.resp[0] & R1_READY_FOR_DATA));

	return ret;
}

/*
 * Transfer a single sector of kernel addressable data
 */
static int mmc_test_buffer_transfer(struct mmc_ucore_card *test,
				    u8 * buffer, unsigned addr, unsigned blksz,
				    int write)
{
	int ret;

	struct mmc_request mrq;
	struct mmc_command cmd;
	struct mmc_command stop;
	struct mmc_data data;

	struct scatterlist sg;

	memset(&mrq, 0, sizeof(struct mmc_request));
	memset(&cmd, 0, sizeof(struct mmc_command));
	memset(&data, 0, sizeof(struct mmc_data));
	memset(&stop, 0, sizeof(struct mmc_command));

	mrq.cmd = &cmd;
	mrq.data = &data;
	mrq.stop = &stop;

	sg_init_one(&sg, buffer, blksz);

	mmc_test_prepare_mrq(test, &mrq, &sg, 1, addr, 1, blksz, write);

	mmc_wait_for_req(test->card->host, &mrq);

	if (cmd.error)
		return cmd.error;
	if (data.error)
		return data.error;

	ret = mmc_test_wait_busy(test);
	if (ret)
		return ret;

	return 0;
}

/*
 * Checks that a normal transfer didn't have any errors
 */
static int mmc_test_check_result(struct mmc_ucore_card *test,
				 struct mmc_request *mrq)
{
	int ret;

	BUG_ON(!mrq || !mrq->cmd || !mrq->data);

	ret = 0;

	if (!ret && mrq->cmd->error)
		ret = mrq->cmd->error;
	if (!ret && mrq->data->error)
		ret = mrq->data->error;
	if (!ret && mrq->stop && mrq->stop->error)
		ret = mrq->stop->error;
	if (!ret && mrq->data->bytes_xfered !=
	    mrq->data->blocks * mrq->data->blksz)
		ret = RESULT_FAIL;

	if (ret == -EINVAL)
		ret = RESULT_UNSUP_HOST;

	return ret;
}

/*
 * Tests a basic transfer with certain parameters
 */
static int mmc_test_simple_transfer(struct mmc_ucore_card *test,
				    struct scatterlist *sg, unsigned sg_len,
				    unsigned dev_addr, unsigned blocks,
				    unsigned blksz, int write)
{
	struct mmc_request mrq;
	struct mmc_command cmd;
	struct mmc_command stop;
	struct mmc_data data;

	memset(&mrq, 0, sizeof(struct mmc_request));
	memset(&cmd, 0, sizeof(struct mmc_command));
	memset(&data, 0, sizeof(struct mmc_data));
	memset(&stop, 0, sizeof(struct mmc_command));

	mrq.cmd = &cmd;
	mrq.data = &data;
	mrq.stop = &stop;

	mmc_test_prepare_mrq(test, &mrq, sg, sg_len, dev_addr,
			     blocks, blksz, write);

	mmc_wait_for_req(test->card->host, &mrq);

	mmc_test_wait_busy(test);

	return mmc_test_check_result(test, &mrq);
}

static int mmc_disk_do_io(struct ide_device *dev, unsigned long secno,
			  void *dst, unsigned long nsecs, int wr)
{
	struct mmc_ucore_card *ucard = (struct mmc_ucore_card *)dev->dev_data;
	int ret = 0;
	struct scatterlist sg;
	//pr_debug("mmc_disk_%s: secno %d, nsces %d\n", wr?"write":"read", secno, nsecs);
	sg_init_one(&sg, dst, nsecs * 512);
	ret =
	    mmc_test_simple_transfer(ucard, &sg, 1, secno << 9, nsecs, 512, wr);
	if (ret)
		return ret;
	return 0;
}

static int mmc_disk_read(struct ide_device *dev, unsigned long secno, void *dst,
			 unsigned long nsecs)
{
	return mmc_disk_do_io(dev, secno, dst, nsecs, 0);
}

static int mmc_disk_write(struct ide_device *dev, unsigned long secno,
			  const void *src, unsigned long nsecs)
{
	return mmc_disk_do_io(dev, secno, src, nsecs, 1);
}

static void mmc_disk_init(struct ide_device *dev)
{
}

static void __init_mmc_ide_device(struct ide_device *dev)
{
	struct mmc_ucore_card *ucard = (struct mmc_ucore_card *)dev->dev_data;
	struct mmc_card *card = ucard->card;
	dev->valid = 1;
	dev->sets = ~0;
	dev->blksz = 512;
	dev->if_type = IF_TYPE_SD;

	if (!mmc_card_sd(card) && mmc_card_blockaddr(card)) {
		/*
		 * The EXT_CSD sector count is in number or 512 byte
		 * sectors.
		 */
		dev->lba = card->ext_csd.sectors;
	} else {
		/*
		 * The CSD capacity field is in units of read_blkbits.
		 * set_capacity takes units of 512 bytes.
		 */
		dev->lba = card->csd.capacity << (card->csd.read_blkbits - 9);
	}
	printk("ucore_mmcblk: lba: %d\n", dev->lba);
	strcpy(dev->model, "mmcblk");
	dev->init = mmc_disk_init;
	dev->read_secs = mmc_disk_read;
	dev->write_secs = mmc_disk_write;
}

static int ucore_mmcblk_probe(struct mmc_card *card)
{
	struct mmc_ucore_card *ucard =
	    kzalloc(sizeof(struct mmc_ucore_card), GFP_KERNEL);
	if (!ucard)
		return -ENOMEM;
	struct ide_device *ucore_dev =
	    kzalloc(sizeof(struct ide_device), GFP_KERNEL);
	if (!ucore_dev) {
		kfree(ucore_dev);
		return -ENOMEM;
	}

	ucore_dev->dev_data = ucard;

	ucard->card = card;
	ucard->ucore_dev = ucore_dev;

	int ret = 0;
	mmc_claim_host(card->host);
	ret = mmc_test_set_blksize(ucard, 512);
	if (ret)
		goto error;

	dev_info(&card->dev, "init MMC block for ucore\n");

	__init_mmc_ide_device(ucore_dev);
	dev_info(&card->dev, "init MMC partitions for ucore\n");

#define MMC0_DEV_NO        3	/* fs.h */
	ret = ide_register_device(MMC0_DEV_NO, ucore_dev);
	if (ret)
		goto error;
	extern void dev_init_mmc0(void);
	dev_init_mmc0();

#define DOS_PBR 1
	ret = test_part_dos(ucore_dev);
	if (ret == DOS_PBR) {
		print_part_dos(ucore_dev);
#ifdef UCONFIG_HAVE_FATFS
		extern void ffs_init();
		ffs_init();
#endif
	} else {
		/* dos mbr or other types */
		printk(KERN_WARNING "Unknown partition type for mmc0\n");
		return 0;
	}

	return 0;
error:
	mmc_release_host(card->host);
	dev_info(&card->dev, "Failed mmc blk (only support PBR) init\n");
	kfree(ucore_dev);
	kfree(ucard);
	return ret;
}

static void ucore_mmcblk_remove(struct mmc_card *card)
{
	mmc_release_host(card->host);
}

static struct mmc_driver mmc_driver = {
	.drv = {
		.name = "ucore_mmcblk",
		},
	.probe = ucore_mmcblk_probe,
	.remove = ucore_mmcblk_remove,
};

static int __init ucore_mmcblk_init(void)
{
	return mmc_register_driver(&mmc_driver);
}

static void __exit ucore_mmcblk_exit(void)
{
	mmc_unregister_driver(&mmc_driver);
}

module_init(ucore_mmcblk_init);
module_exit(ucore_mmcblk_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chen Yuheng");
