#ifndef __DRIVER_OS_H__
#define __DRIVER_OS_H__

#include <types.h>

extern volatile char *driver_os_buffer;
extern size_t driver_os_buffer_size;

void driver_os_init(void);
void driver_os_notify(void);

#endif
