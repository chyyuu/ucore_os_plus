#ifndef __BOOT_ALLOC_H__
#define __BOOT_ALLOC_H__

#include <types.h>

void boot_alloc_init(void);
void *boot_alloc(uintptr_t size, uintptr_t align, int verbose);
uintptr_t boot_alloc_get_free(void);
#endif
