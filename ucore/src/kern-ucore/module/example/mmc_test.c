/*
 * =====================================================================================
 *
 *       Filename:  mmc_test.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  08/15/2012 09:38:32 PM
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

#define RESULT_OK		0
#define RESULT_FAIL		1
#define RESULT_UNSUP_HOST	2
#define RESULT_UNSUP_CARD	3

#define BUFFER_ORDER		2
#define BUFFER_SIZE		(PAGE_SIZE << BUFFER_ORDER)

struct mmc_test_card {
	struct mmc_card *card;

	u8 scratch[BUFFER_SIZE];
	u8 *buffer;
#ifdef CONFIG_HIGHMEM
	struct page *highmem;
#endif
};

static ssize_t mmc_test_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{

	return 0;
}

static ssize_t mmc_test_store(struct device *dev,
			      struct device_attribute *attr, const char *buf,
			      size_t count)
{
	return 0;
}

/*
 * Configure correct block size in card
 */
static int mmc_test_set_blksize(struct mmc_test_card *test, unsigned size)
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
static void mmc_test_prepare_mrq(struct mmc_test_card *test,
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
static int mmc_test_wait_busy(struct mmc_test_card *test)
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
static int mmc_test_buffer_transfer(struct mmc_test_card *test,
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

/*******************************************************************/
/*  Test preparation and cleanup                                   */
/*******************************************************************/

/*
 * Fill the first couple of sectors of the card with known data
 * so that bad reads/writes can be detected
 */
static int __mmc_test_prepare(struct mmc_test_card *test, int write)
{
	int ret, i;

	ret = mmc_test_set_blksize(test, 512);
	if (ret)
		return ret;

	if (write)
		memset(test->buffer, 0xDF, 512);
	else {
		for (i = 0; i < 512; i++)
			test->buffer[i] = i;
	}

	for (i = 0; i < BUFFER_SIZE / 512; i++) {
		ret =
		    mmc_test_buffer_transfer(test, test->buffer, i * 512, 512,
					     1);
		if (ret)
			return ret;
	}

	return 0;
}

static int mmc_test_prepare_write(struct mmc_test_card *test)
{
	return __mmc_test_prepare(test, 1);
}

static int mmc_test_prepare_read(struct mmc_test_card *test)
{
	return __mmc_test_prepare(test, 0);
}

static int mmc_test_cleanup(struct mmc_test_card *test)
{
	int ret, i;

	ret = mmc_test_set_blksize(test, 512);
	if (ret)
		return ret;

	memset(test->buffer, 0, 512);

	for (i = 0; i < BUFFER_SIZE / 512; i++) {
		ret =
		    mmc_test_buffer_transfer(test, test->buffer, i * 512, 512,
					     1);
		if (ret)
			return ret;
	}

	return 0;
}

/*
 * Checks that a normal transfer didn't have any errors
 */
static int mmc_test_check_result(struct mmc_test_card *test,
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
static int mmc_test_simple_transfer(struct mmc_test_card *test,
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

static int mmc_test_basic_write(struct mmc_test_card *test)
{
	int ret;
	struct scatterlist sg;

	ret = mmc_test_set_blksize(test, 512);
	if (ret)
		return ret;

	sg_init_one(&sg, test->buffer, 512);

	ret = mmc_test_simple_transfer(test, &sg, 1, 0, 1, 512, 1);
	if (ret)
		return ret;

	return 0;
}

static int mmc_test_basic_read(struct mmc_test_card *test)
{
	int ret;
	struct scatterlist sg;

	ret = mmc_test_set_blksize(test, 512);
	if (ret)
		return ret;

	memset(test->buffer, 0, 512);
	sg_init_one(&sg, test->buffer, 512);

	ret = mmc_test_simple_transfer(test, &sg, 1, 0, 1, 512, 0);
	if (ret)
		return ret;

	return 0;
}

static int test_now(struct mmc_card *card)
{
	struct mmc_test_card *test = NULL;
	test = kzalloc(sizeof(struct mmc_test_card), GFP_KERNEL);
	if (!test)
		return -ENOMEM;

	test->card = card;

	test->buffer = kzalloc(BUFFER_SIZE, GFP_KERNEL);

	dev_info(&card->dev, "MMC start testing...\n");
	mmc_claim_host(card->host);

	int i, bad = 0, ret;
#if 0
	for (i = 0; i < 512; i++)
		test->buffer[i] = (u8) i;

	ret = mmc_test_basic_write(test);
	dev_info(&card->dev, "MMC basic wr test... %d\n", ret);
#endif

	ret = mmc_test_basic_read(test);
	dev_info(&card->dev, "MMC basic rd test... %d\n", ret);

	for (i = 0; i < 512; i++)
		if (test->buffer[i] != (u8) i) {
			bad = 1;
			break;
		}

	if (bad) {
		dev_info(&card->dev, "MMC check test failed... %d\n", i);
	} else {
		dev_info(&card->dev, "MMC check done\n");
	}

	mmc_release_host(card->host);
	dev_info(&card->dev, "MMC test done...\n");
}

static DEVICE_ATTR(test, S_IWUSR | S_IRUGO, mmc_test_show, mmc_test_store);
static int mmc_test_probe(struct mmc_card *card)
{
	int ret;

	if ((card->type != MMC_TYPE_MMC) && (card->type != MMC_TYPE_SD))
		return -ENODEV;

	ret = device_create_file(&card->dev, &dev_attr_test);
	if (ret)
		return ret;

	dev_info(&card->dev, "Card claimed for testing.\n");
	test_now(card);

	return 0;
}

static void mmc_test_remove(struct mmc_card *card)
{
	device_remove_file(&card->dev, &dev_attr_test);
}

static struct mmc_driver mmc_driver = {
	.drv = {
		.name = "mmc_test",
		},
	.probe = mmc_test_probe,
	.remove = mmc_test_remove,
};

static int __init mmc_test_init(void)
{
	return mmc_register_driver(&mmc_driver);
}

static void __exit mmc_test_exit(void)
{
	mmc_unregister_driver(&mmc_driver);
}

module_init(mmc_test_init);
module_exit(mmc_test_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Multimedia Card (MMC) host test driver");
MODULE_AUTHOR("Chen Yuheng");
