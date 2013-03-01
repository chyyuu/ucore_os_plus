#include <types.h>
#include <stdio.h>
#include <trap.h>
//#include <picirq.h>
#include <fs.h>
#include <ide.h>
#include <arch.h>
#include <sem.h>
#include <assert.h>
#include <kio.h>
#include <flash.h>

void ide_init(void)
{
	flash_init();
}

bool ide_device_valid(unsigned short ideno)
{
	switch (ideno) {
	case 1:		//swap
		return 1;
	case 2:		//file system on flash
		return 1;
	default:
		return 0;
	}
}

size_t ide_device_size(unsigned short ideno)
{
	switch (ideno) {
	case 1:		//swap
		return 512 * 8;
	case 2:		//file system on flash
		return flash_size();
	default:
		return 0;
	}
}

int ide_read_secs(unsigned short ideno, uint32_t secno, void *dst, size_t nsecs)
{
	if (ideno == 2) {
		return flash_read_secs(secno, dst, nsecs);
	} else {
		panic("read sec : ideno=%d\n", ideno);
	}
}

int
ide_write_secs(unsigned short ideno, uint32_t secno, const void *src,
	       size_t nsecs)
{
	if (ideno == 2) {
		return flash_write_secs(secno, src, nsecs);
	} else {
		panic("write sec : ideno=%d\n", ideno);
	}
	return 0;
}
