#ifndef __KERN_MM_SLAB_H__
#define __KERN_MM_SLAB_H__

#include <types.h>

#ifndef KMALLOC_MAX_ORDER
#define KMALLOC_MAX_ORDER       10
#endif

void slab_init(void);

void *kmalloc(size_t n);
void kfree(void *objp);

size_t slab_allocated(void);

#endif /* !__KERN_MM_SLAB_H__ */
