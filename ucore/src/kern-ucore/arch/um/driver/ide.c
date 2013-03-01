#include <ide.h>
#include <types.h>
#include <stdio.h>
#include <trap.h>
#include <fs.h>
#include <assert.h>
#include <memlayout.h>
#include <arch.h>
#include <sem.h>
#include <kio.h>

#define MAX_IDE                 4
#define MAX_NSECS               128
#define MAX_DISK_NSECS          0x10000000U
#define VALID_IDE(ideno)        (((ideno) >= 0) && ((ideno) < MAX_IDE) && (ide_devices[ideno].valid))

static struct {
	semaphore_t sem;
} channels[2];

static void lock_channel(unsigned short ideno)
{
	down(&(channels[ideno >> 1].sem));
}

static void unlock_channel(unsigned short ideno)
{
	up(&(channels[ideno >> 1].sem));
}

static struct ide_device {
	unsigned int fd;
	unsigned char valid;	// 0 or 1 (If Device Really Exists)
	unsigned int size;	// Size in Sectors
} ide_devices[MAX_IDE];

void ide_init(void)
{
	if (ginfo->disk_fd > 0) {
		ide_devices[DISK0_DEV_NO].fd = ginfo->disk_fd;
		ide_devices[DISK0_DEV_NO].valid = 1;
		ide_devices[DISK0_DEV_NO].size = ginfo->disk_size;
		kprintf("disk file size: %d (sectors)\n", ginfo->swap_size);
	} else {
		ide_devices[DISK0_DEV_NO].valid = 0;
	}

	if (ginfo->swap_fd > 0) {
		ide_devices[SWAP_DEV_NO].fd = ginfo->swap_fd;
		ide_devices[SWAP_DEV_NO].valid = 1;
		ide_devices[SWAP_DEV_NO].size = ginfo->swap_size;
		kprintf("swap file size: %d (sectors)\n", ginfo->swap_size);
	} else {
		ide_devices[SWAP_DEV_NO].valid = 0;
	}

	sem_init(&(channels[0].sem), 1);
	sem_init(&(channels[1].sem), 1);
}

bool ide_device_valid(unsigned short ideno)
{
	return VALID_IDE(ideno);
}

size_t ide_device_size(unsigned short ideno)
{
	if (ide_device_valid(ideno)) {
		return ide_devices[ideno].size;
	}
	return 0;
}

int ide_read_secs(unsigned short ideno, uint32_t secno, void *dst, size_t nsecs)
{
	assert(nsecs <= MAX_NSECS && VALID_IDE(ideno));
	assert(secno < MAX_DISK_NSECS && secno + nsecs <= MAX_DISK_NSECS);

	int ret;
	lock_channel(ideno);
	ret =
	    syscall3(__NR_lseek, ide_devices[ideno].fd, secno * SECTSIZE,
		     SEEK_SET);
	if (ret != -1)
		ret =
		    syscall3(__NR_read, ide_devices[ideno].fd, (long)dst,
			     nsecs * SECTSIZE);
	unlock_channel(ideno);

	if (ret > 0)
		return 0;
	return ret;
}

int
ide_write_secs(unsigned short ideno, uint32_t secno, const void *src,
	       size_t nsecs)
{
	assert(nsecs <= MAX_NSECS && VALID_IDE(ideno));
	assert(secno < MAX_DISK_NSECS && secno + nsecs <= MAX_DISK_NSECS);

	int ret;
	lock_channel(ideno);
	ret =
	    syscall3(__NR_lseek, ide_devices[ideno].fd, secno * SECTSIZE,
		     SEEK_SET);
	if (ret != -1)
		ret =
		    syscall3(__NR_write, ide_devices[ideno].fd, (long)src,
			     nsecs * SECTSIZE);
	unlock_channel(ideno);

	if (ret > 0)
		return 0;
	return ret;
}
