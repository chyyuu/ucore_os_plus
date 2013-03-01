/*
 * =====================================================================================
 *
 *       Filename:  dev_disk1.c
 *
 *    Description:  Nand flash disk
 *
 *        Version:  1.0
 *        Created:  04/09/2012 04:33:21 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#include <stdlib.h>

/*
 * Implementation of the disk1 device, "disk1:", which generates an
 * immediate EOF on read and throws away anything written to it.
 */
/* Nand flash Disk Driver 
 * Only create a dev-inode here
 * initialization should have been done by hardware-dependent code
 */

#include <types.h>
#include <dev.h>
#include <vfs.h>
#include <iobuf.h>
#include <inode.h>
#include <error.h>
#include <assert.h>

/* For open() */
static int disk1_open(struct device *dev, uint32_t open_flags)
{
	return 0;
}

/* For close() */
static int disk1_close(struct device *dev)
{
	return 0;
}

/* For dop_io() */
static int disk1_io(struct device *dev, struct iobuf *iob, bool write)
{
	return -E_INVAL;
}

/* For ioctl() */
static int disk1_ioctl(struct device *dev, int op, void *data)
{
	return -E_INVAL;
}

static void disk1_device_init(struct device *dev)
{
	memset(dev, 0, sizeof(*dev));
	dev->d_blocks = 0;
	dev->d_blocksize = 1;
	dev->d_open = disk1_open;
	dev->d_close = disk1_close;
	dev->d_io = disk1_io;
	dev->d_ioctl = disk1_ioctl;
}

/*
 * Function to create and attach disk1:
 */
void dev_init_disk1(void)
{
	struct inode *node;
	if ((node = dev_create_inode()) == NULL) {
		panic("disk1: dev_create_node.\n");
	}
	disk1_device_init(vop_info(node, device));

	int ret;
	if ((ret = vfs_add_dev("disk1", node, 1)) != 0) {
		panic("disk1: vfs_add_dev: %e.\n", ret);
	}
}
