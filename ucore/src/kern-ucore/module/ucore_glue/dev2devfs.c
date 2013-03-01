/*
 * =====================================================================================
 *
 *       Filename:  dev2devfs_glue.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/21/2012 05:17:37 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <linux/types.h>
#include <linux/kdev_t.h>

#define __NO_UCORE_TYPE__
#include <vfs.h>
#include <dev.h>
#include <inode.h>
#include <error.h>
#include <assert.h>
#include <kio.h>
#include <stat.h>
#include <error.h>

extern const struct file_operations def_chr_fops;

/* in helper */
extern int __ucore_linux_inode_fops_stub_open(struct device *dev,
					      uint32_t open_flags);
extern int __ucore_linux_inode_fops_stub_close(struct device *dev);
extern int __ucore_linux_inode_fops_stub_ioctl(struct device *dev,
					       unsigned int cmd,
					       unsigned long arg);
extern int __ucore_linux_inode_fops_stub_read(struct device *dev,
					      const char __user * buf,
					      size_t count, size_t * offset);
extern int __ucore_linux_inode_fops_stub_write(struct device *dev,
					       const char __user * buf,
					       size_t count, size_t * offset);
extern int __ucore_linux_inode_fops_stub_mmap2(struct device *dev, void *addr,
					       size_t len, int unused1,
					       int unused2, size_t pgoff);

static int
__ucore_vfs_device_caller_io(struct device *dev, struct iobuf *iob, bool write)
{
	kprintf("## NEVER call __ucore_vfs_device_caller_io!\n");
	return -E_INVAL;
}

static void
__ucore_vfs_device_init(struct device *dev, dev_t devno, mode_t mode)
{
	const struct file_operations *fops = NULL;
	memset(dev, 0, sizeof(*dev));
	if (S_ISCHR(mode)) {
		fops = &def_chr_fops;
	} else {
		kprintf("__ucore_vfs_device_init: Warning: unknown mode %o\n",
			mode);
	}
	dev->linux_dentry = __ucore_create_linux_dentry(devno, mode, fops);

	dev->d_open = __ucore_linux_inode_fops_stub_open;
	dev->d_close = __ucore_linux_inode_fops_stub_close;
	dev->d_io = __ucore_vfs_device_caller_io;

	/* linux */
	dev->d_linux_write = __ucore_linux_inode_fops_stub_write;
	dev->d_linux_read = __ucore_linux_inode_fops_stub_read;
	dev->d_linux_ioctl = __ucore_linux_inode_fops_stub_ioctl;
	dev->d_linux_mmap = __ucore_linux_inode_fops_stub_mmap2;
}

void ucore_vfs_add_device(const char *name, int major, int minor)
{
	struct inode *node;
	if ((node = dev_create_inode()) == NULL) {
		panic("ucore_vfs_add_device: dev_create_node %s.\n", name);
	}

	__ucore_vfs_device_init(vop_info(node, device), MKDEV(major, minor),
				S_IFCHR);

	int ret;
	if ((ret = vfs_add_dev(name, node, 0)) != 0) {
		panic("ucore_vfs_add_device: vfs_add_dev %s: %e.\n", name, ret);
	}
	kprintf("ucore_vfs_add_device: %s %d:%d\n", name, major, minor);
}
