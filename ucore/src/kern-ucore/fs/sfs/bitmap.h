#ifndef __KERN_FS_SFS_BITMAP_H__
#define __KERN_FS_SFS_BITMAP_H__

#include <types.h>

struct bitmap;

struct bitmap *bitmap_create(uint32_t nbits);
int bitmap_alloc(struct bitmap *bitmap, uint32_t * index_store);
bool bitmap_test(struct bitmap *bitmap, uint32_t index);
void bitmap_free(struct bitmap *bitmap, uint32_t index);
void bitmap_destroy(struct bitmap *bitmap);
void *bitmap_getdata(struct bitmap *bitmap, size_t * len_store);

#endif /* !__KERN_FS_SFS_BITMAP_H__ */
