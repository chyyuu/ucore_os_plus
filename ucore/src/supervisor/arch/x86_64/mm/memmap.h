#ifndef __MEMMAP_H__
#define __MEMMAP_H__

#include <types.h>

/* Memmap use a simple array to store the data */
/* Enlarge the size of array if the data overflowed */
#define MEMMAP_SCALE 400

/* Same with e820 spec */
#define MEMMAP_FLAG_FREE        1
#define MEMMAP_FLAG_RESERVED    2
#define MEMMAP_FLAG_RECLAIMABLE 3
#define MEMMAP_FLAG_NVS         4
#define MEMMAP_FLAG_BAD         5

void memmap_reset(void);
int memmap_append(uintptr_t start, uintptr_t end, int flag);
void memmap_process(int verbose);

int memmap_test(uintptr_t start, uintptr_t end);
int memmap_enumerate(int num, uintptr_t * start, uintptr_t * end);

#endif
