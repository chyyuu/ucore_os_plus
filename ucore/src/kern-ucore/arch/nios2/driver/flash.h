#ifndef __NIOS2_DRIVER_FLASH_H__
#define __NIOS2_DRIVER_FLASH_H__

#include <types.h>

void flash_init(void);

size_t flash_size();

int flash_read_secs(uint32_t secno, void *dst, size_t nsecs);

int flash_write_secs(uint32_t secno, void *dst, size_t nsecs);

#endif /* !__NIOS2_DRIVER_FLASH_H__ */
