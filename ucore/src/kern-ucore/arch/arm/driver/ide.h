#ifndef __KERN_DRIVER_RAMDISK_H__
#define __KERN_DRIVER_RAMDISK_H__

void ide_init(void);
int ide_device_valid(unsigned short ideno);
unsigned long ide_device_size(unsigned short ideno);

int ide_read_secs(unsigned short ideno, unsigned long secno, void *dst,
		  unsigned long nsecs);
int ide_write_secs(unsigned short ideno, unsigned long secno, const void *src,
		   unsigned long nsecs);

#ifdef UCONFIG_SYS_64BIT_LBA
typedef uint64_t lbaint_t;
#else
typedef unsigned long lbaint_t;
#endif

struct ide_device {
	unsigned char valid;	// 0 or 1 (If Device Really Exists)

	int if_type;		/* type of the interface */
	int dev;		/* device number */
	unsigned char part_type;	/* partition type */
	unsigned char target;	/* target SCSI ID */
	unsigned char lun;	/* target LUN */
	unsigned char type;	/* device type */
	unsigned char removable;	/* removable device */
#ifdef UCONFIG_LBA48
	unsigned char lba48;	/* device can use 48bit addr (ATA/ATAPI v7) */
#endif
	lbaint_t lba;		/* number of blocks */
	unsigned long blksz;	/* block size */
	char vendor[40 + 1];	/* IDE model, SCSI Vendor */
	char product[20 + 1];	/* IDE Serial no, SCSI product */
	char revision[8 + 1];	/* firmware revision */

	unsigned int sets;	// Commend Sets Supported
	void *iobase;
	void *dev_data;
	char model[41];		// Model in String

	void (*init) (struct ide_device * dev);
	/* return 0 if succeed */
	int (*read_secs) (struct ide_device * dev, unsigned long secno,
			  void *dst, unsigned long nsecs);
	int (*write_secs) (struct ide_device * dev, unsigned long secno,
			   const void *src, unsigned long nsecs);

};

/* Interface types: */
#define IF_TYPE_UNKNOWN		0
#define IF_TYPE_IDE		1
#define IF_TYPE_SCSI		2
#define IF_TYPE_ATAPI		3
#define IF_TYPE_USB		4
#define IF_TYPE_DOC		5
#define IF_TYPE_MMC		6
#define IF_TYPE_SD		7
#define IF_TYPE_SATA		8

/* Part types */
#define PART_TYPE_UNKNOWN	0x00
#define PART_TYPE_MAC		0x01
#define PART_TYPE_DOS		0x02
#define PART_TYPE_ISO		0x03
#define PART_TYPE_AMIGA		0x04
#define PART_TYPE_EFI		0x05

/*
 * Type string for U-Boot bootable partitions
 */
#define BOOT_PART_TYPE	"U-Boot"	/* primary boot partition type  */
#define BOOT_PART_COMP	"PPCBoot"	/* PPCBoot compatibility type   */

/* device types */
#define DEV_TYPE_UNKNOWN	0xff	/* not connected */
#define DEV_TYPE_HARDDISK	0x00	/* harddisk */
#define DEV_TYPE_TAPE		0x01	/* Tape */
#define DEV_TYPE_CDROM		0x05	/* CD-ROM */
#define DEV_TYPE_OPDISK		0x07	/* optical disk */

typedef struct ucore_disk_partition {
	unsigned long start;	/* # of first block in partition        */
	unsigned long size;	/* number of blocks in partition        */
	unsigned long blksz;	/* block size in bytes                  */
	unsigned char name[32];	/* partition name                       */
	unsigned char type[32];	/* string type description              */
} ucore_disk_partition_t;

int ide_register_device(unsigned short ideno, struct ide_device *dev);

#endif /* !__KERN_DRIVER_RAMDISK_H__ */
