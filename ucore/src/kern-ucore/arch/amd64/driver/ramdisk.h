#ifndef __DRIVERS_RAMDISK_H
#define __DRIVERS_RAMDISK_H

/* defined in init.c */
extern char *initrd_begin, *initrd_end;

#define CHECK_INITRD_EXIST() (initrd_end != initrd_begin)
#define INITRD_SIZE() (initrd_end-initrd_begin)

struct ide_device;
void ramdisk_init_struct(struct ide_device *dev);
void ramdisk_init(struct ide_device *dev);

int ramdisk_read(struct ide_device *dev, unsigned long secno, void *dst,
		unsigned long nsecs);

int ramdisk_write(struct ide_device *dev, unsigned long secno,
			 const void *src, unsigned long nsecs);
#endif
