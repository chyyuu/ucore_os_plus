#ifndef __KERN_DRIVER_RAMDISK_H__
#define __KERN_DRIVER_RAMDISK_H__

#include <defs.h>

void ide_init(void);
bool ide_device_valid(unsigned int ideno);
size_t ide_device_size(unsigned int ideno);

int ide_read_secs(unsigned int ideno, uint32_t secno, void *dst, size_t nsecs);
int ide_write_secs(unsigned int ideno, uint32_t secno, const void *src,
		   size_t nsecs);

struct ide_device {
	unsigned int valid;	// 0 or 1 (If Device Really Exists)
	unsigned int sets;	// Commend Sets Supported
	unsigned int size;	// Size in Sectors
	uintptr_t iobase;
	void *dev_data;
	char model[32];		// Model in String

	void (*init) (struct ide_device * dev);
	/* return 0 if succeed */
	int (*read_secs) (struct ide_device * dev, size_t secno, void *dst,
			  size_t nsecs);
	int (*write_secs) (struct ide_device * dev, size_t secno,
			   const void *src, size_t nsecs);

};

#endif /* !__KERN_DRIVER_RAMDISK_H__ */
