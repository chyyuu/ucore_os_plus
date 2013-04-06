/*
 * =====================================================================================
 *
 *       Filename:  ramdisk.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/26/2012 06:16:29 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <string.h>
#include <memlayout.h>
#include <assert.h>
#include <stdio.h>
#include <kio.h>
#include <fs.h>
#include <ramdisk.h>
#include <ide.h>

#define MIN(x,y) (((x)<(y))?(x):(y))

int ramdisk_read(struct ide_device *dev, unsigned long secno, void *dst,
			unsigned long nsecs)
{
	nsecs = MIN(nsecs, dev->size - secno);
	if (nsecs < 0)
		return -1;
	memcpy(dst, (void *)(initrd_begin + secno * SECTSIZE), nsecs * SECTSIZE);
	return 0;
}

int ramdisk_write(struct ide_device *dev, unsigned long secno,
			 const void *src, unsigned long nsecs)
{
	//kprintf("%08x(%d) %08x(%d)\n", dev->size, dev->size, secno, secno);
	nsecs = MIN(nsecs, dev->size - secno);
	if (nsecs < 0)
		return -1;
	memcpy((void *)(initrd_begin + secno * SECTSIZE), src, nsecs * SECTSIZE);
	return 0;
}

void ramdisk_init(struct ide_device *dev)
{
	kprintf("ramdisk_init(): initrd found, magic: %p, 0x%08x secs\n",
	initrd_begin, dev->size);

}

void ramdisk_init_struct(struct ide_device *dev)
{
	memset(dev, 0, sizeof(struct ide_device));
	assert(INITRD_SIZE() % SECTSIZE == 0);
	if (CHECK_INITRD_EXIST()) {
		dev->valid = 1;
		dev->sets = ~0;
		dev->ramdisk = 1;
		dev->size = INITRD_SIZE() / SECTSIZE;
		//dev->iobase = (void *) DISK_FS_VBASE;
		strcpy(dev->model, "KERN_INITRD");
		//dev->init = ramdisk_init;
		//dev->read_secs = ramdisk_read;
		//dev->write_secs = ramdisk_write;
	}
}
