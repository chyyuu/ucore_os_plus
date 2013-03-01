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

#include <defs.h>
#include <ramdisk.h>
#include <string.h>
#include <memlayout.h>
#include <assert.h>
#include <stdio.h>
#include <fs.h>

#define MIN(x,y) (((x)<(y))?(x):(y))

//char initrd_begin[], initrd_end[];

bool check_initrd()
{
	if (_initrd_begin == _initrd_end) {
		kprintf("Warning: No Initrd!\n");
		return 0;
	}
	kprintf("Initrd: 0x%08x - 0x%08x, size: 0x%08x, magic: 0x%08x\n",
		_initrd_begin, _initrd_end - 1, _initrd_end - _initrd_begin,
		*(uint32_t *) _initrd_begin);
	return 1;
}

static int ramdisk_read(struct ide_device *dev, size_t secno, void *dst,
			size_t nsecs)
{
	nsecs = MIN(nsecs, dev->size - secno);
	if (nsecs < 0)
		return -1;
	memcpy(dst, (void *)(dev->iobase + secno * SECTSIZE), nsecs * SECTSIZE);
	return 0;
}

static int ramdisk_write(struct ide_device *dev, size_t secno, const void *src,
			 size_t nsecs)
{
	//kprintf("%08x(%d) %08x(%d)\n", dev->size, dev->size, secno, secno);
	nsecs = MIN(nsecs, dev->size - secno);
	if (nsecs < 0)
		return -1;
	memcpy((void *)(dev->iobase + secno * SECTSIZE), src, nsecs * SECTSIZE);
	return 0;
}

static void ramdisk_init(struct ide_device *dev)
{
	kprintf("ramdisk_init(): initrd found, magic: 0x%08x, 0x%08x secs\n",
		*(uint32_t *) (dev->iobase), dev->size);

}

void ramdisk_init_struct(struct ide_device *dev)
{
	memset(dev, 0, sizeof(struct ide_device));
	assert(INITRD_SIZE() % SECTSIZE == 0);
	if (CHECK_INITRD_EXIST()) {
		dev->valid = 1;
		dev->sets = ~0;
		dev->size = INITRD_SIZE() / SECTSIZE;
		dev->iobase = (uintptr_t) _initrd_begin;
		strcpy(dev->model, "KERN_INITRD");
		dev->init = ramdisk_init;
		dev->read_secs = ramdisk_read;
		dev->write_secs = ramdisk_write;
	}
}
