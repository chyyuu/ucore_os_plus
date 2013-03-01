#include <stdio.h>
#include <arch.h>
#include <trap.h>
#include <picirq.h>
#include <fs.h>
#include <ide.h>
#include <assert.h>
#include <ramdisk.h>
#include <memlayout.h>

#define VALID_IDE(ideno)        (((ideno) >= 0) && ((ideno) < MAX_IDE) && (ide_devices[ideno].valid))

#define MAX_IDE 4

#define CHECK_CALL(ideno, func) (VALID_IDE(ideno) && ide_devices[ideno].func)

//#define _CHECK_NANDFLASH

static struct ide_device ide_devices[MAX_IDE];

static int ide_wait_ready(unsigned int iobase, bool check_error)
{
	return 0;
}

// TODO: check if ramdisk are there
void ide_init(void)
{
	int i;
	for (i = 0; i < MAX_IDE; i++)
		ide_devices[i].valid = 0;
	int devno = DISK0_DEV_NO;
	assert(devno < MAX_IDE);
	ramdisk_init_struct(&ide_devices[devno]);
	if (CHECK_CALL(devno, init))
		ide_devices[devno].init(&ide_devices[devno]);

}

bool ide_device_valid(unsigned int ideno)
{
	return VALID_IDE(ideno);
}

size_t ide_device_size(unsigned int ideno)
{
	if (ide_device_valid(ideno)) {
		return ide_devices[ideno].size;
	}
	return 0;
}

int ide_read_secs(unsigned int ideno, uint32_t secno, void *dst, size_t nsecs)
{
	if (CHECK_CALL(ideno, read_secs))
		return ide_devices[ideno].read_secs(&ide_devices[ideno], secno,
						    dst, nsecs);
	return 0;
}

int
ide_write_secs(unsigned int ideno, uint32_t secno, const void *src,
	       size_t nsecs)
{
	if (CHECK_CALL(ideno, write_secs))
		return ide_devices[ideno].write_secs(&ide_devices[ideno], secno,
						     src, nsecs);
	return 0;
}
