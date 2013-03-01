/*
 * =====================================================================================
 *
 *       Filename:  char_example.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/22/2012 10:31:33 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>

#define MAJOR_NUM 222
#define MINOR_NUM 0
#define DEV_NAME "hzfchar"

static int hzf_var = 0;
static int hzf_major = MAJOR_NUM;
static int hzf_minor = MINOR_NUM;
static char *pname = DEV_NAME;

static ssize_t hzf_read(struct file *file, char __user * buf, size_t count,
			loff_t * offset)
{
	if (copy_to_user(buf, &hzf_var, sizeof(hzf_var)))
		return -EFAULT;

	printk(KERN_INFO "hzf chrdev offset=%lld readvar=%d\n", *offset,
	       hzf_var);
	return sizeof(hzf_var);
}

static ssize_t hzf_write(struct file *file, const char __user * buf,
			 size_t count, loff_t * offset)
{
	if (copy_from_user(&hzf_var, buf, sizeof(int)))
		return -EFAULT;

	printk(KERN_INFO "hzf chrdev offset=%lld writenew var=%d\n", *offset,
	       hzf_var);
	return sizeof(int);
}

struct hzf_dev {
	int data;
	struct cdev cdev;
};

struct hzf_dev my_cdev;

static struct file_operations hzf_cdev_ops = {
	.owner = THIS_MODULE,
	.read = hzf_read,
	.write = hzf_write
};

static int __init hzf_init(void)
{
	int ret = 0;
	dev_t dev = 0;

	if (hzf_major) {
		dev = MKDEV(hzf_major, hzf_minor);
		ret = register_chrdev_region(dev, 1, pname);
	} else {
		ret = alloc_chrdev_region(&dev, 0, 1, pname);
	}

	if (ret)
		goto fail0;

	cdev_init(&my_cdev.cdev, &hzf_cdev_ops);
	my_cdev.cdev.owner = THIS_MODULE;
	my_cdev.cdev.ops = &hzf_cdev_ops;

	ret = cdev_add(&my_cdev.cdev, dev, 1);
	if (ret)
		goto fail1;

	printk("char example register success!\n");
	return 0;

fail1:
	printk("cdev_addfail!\n");
	unregister_chrdev_region(dev, 1);
	return -1;
fail0:
	printk("register chardevice region fail!\n");
	return -2;
}

static void __exit hzf_cleanup(void)
{

	int ret = 0;
	dev_t dev = MKDEV(hzf_major, hzf_minor);

	cdev_del(&my_cdev.cdev);
	unregister_chrdev_region(dev, 1);

	if (ret)
		printk("unregisterhzfchar fail!\n");
	else
		printk("unregistersuccess!\n");
}

module_init(hzf_init);
module_exit(hzf_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stephen Hu");
MODULE_DESCRIPTION("A simple module of char device driver");
