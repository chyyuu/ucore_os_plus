/*
 * =====================================================================================
 *
 *       Filename:  dev2devfs_glue_helper.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/21/2012 09:56:06 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <linux/fs.h>
#include <linux/mm.h>

#define __NO_UCORE_TYPE__
#define __NO_UCORE_DEVICE__
#include <dev.h>
#include <assert.h>

void *__ucore_create_linux_dentry(dev_t devno, mode_t mode,
				  const struct file_operations *fops)
{
	struct inode *n = kzalloc(sizeof(struct inode), GFP_KERNEL);
	if (!n)
		return NULL;
	struct dentry *d = kzalloc(sizeof(struct dentry), GFP_KERNEL);
	if (!d) {
		kfree(n);
		return NULL;
	}

	n->i_fop = fops;
	n->i_rdev = devno;
	n->i_mode = mode;

	d->d_inode = n;
	strcpy((char *)d->d_iname, "linuxdev");

	return d;
}

static void *__ucore_create_linux_file(struct ucore_device *dev)
{
	struct file *f = kzalloc(sizeof(struct file), GFP_KERNEL);
	if (!f)
		return NULL;
	assert(dev->linux_dentry);
	f->f_path.dentry = (struct inode *)dev->linux_dentry;
	return f;
}

int __ucore_linux_inode_fops_stub_open(struct ucore_device *dev,
				       uint32_t open_flags)
{
	assert(dev->linux_dentry);
	struct dentry *d = (struct dentry *)dev->linux_dentry;
	int ret = -EINVAL;
	/* we do not support open linux devfile twice */
	if (dev->linux_file)
		return -EPERM;
	dev->linux_file = __ucore_create_linux_file(dev);
	if (!dev->linux_file)
		return -ENOMEM;
	struct inode *node = d->d_inode;
	struct file *file = (struct file *)dev->linux_file;
	//kprintf("### f->fop %08x\n", file->f_op);
	if (!node->i_fop->open)
		return ret;
	ret = node->i_fop->open(node, file);
	//kprintf("### f->fop %08x\n", file->f_op);
	return ret;
}

int __ucore_linux_inode_fops_stub_close(struct ucore_device *dev)
{
	kfree(dev->linux_file);
	dev->linux_file = NULL;
	return 0;
}

int __ucore_linux_inode_fops_stub_ioctl(struct ucore_device *dev,
					unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;
	//struct inode *node = (struct inode*)dev->linux_inode;
	struct file *file = (struct file *)dev->linux_file;
	assert(file);
	if (file->f_op && file->f_op->unlocked_ioctl)
		ret = file->f_op->unlocked_ioctl(file, cmd, arg);
	return ret;
}

int __ucore_linux_inode_fops_stub_read(struct ucore_device *dev,
				       const char __user * buf, size_t count,
				       size_t * offset)
{
	int ret = -EINVAL;
	//struct inode *node = (struct inode*)dev->linux_inode;
	struct file *file = (struct file *)dev->linux_file;
	assert(file);
	if (file->f_op && file->f_op->read)
		ret = file->f_op->read(file, buf, count, (loff_t *) offset);
	return ret;
}

int __ucore_linux_inode_fops_stub_write(struct ucore_device *dev,
					const char __user * buf, size_t count,
					size_t * offset)
{
	int ret = -EINVAL;
	//struct inode *node = (struct inode*)dev->linux_inode;
	struct file *file = (struct file *)dev->linux_file;
	assert(file);
	if (file->f_op && file->f_op->write)
		ret = file->f_op->write(file, buf, count, (loff_t *) offset);
	return ret;
}

void *__ucore_linux_inode_fops_stub_mmap2(struct ucore_device *dev, void *addr,
					  size_t len, int unused1, int unused2,
					  size_t pgoff)
{
	int ret = -EINVAL;
	struct vm_area_struct *vma = NULL;
	vma = kzalloc(sizeof(struct vm_area_struct), GFP_KERNEL);
	if (!vma)
		return NULL;
	//struct inode *node = (struct inode*)dev->linux_inode;
	struct file *file = (struct file *)dev->linux_file;
	//unsigned long pgoff = (off & PAGE_MASK) >> PAGE_SHIFT;
	assert(file);
	len = PAGE_ALIGN(len);
	vma->vm_start = addr;
	vma->vm_end = addr + len;
	vma->vm_pgoff = pgoff;
	if (file->f_op && file->f_op->mmap)
		ret = file->f_op->mmap(file, vma);
	void *r = vma->vm_start;
	kfree(vma);
	if (ret)
		return NULL;
	//kprintf("## dd %08x\n", r);
	return r;
}
