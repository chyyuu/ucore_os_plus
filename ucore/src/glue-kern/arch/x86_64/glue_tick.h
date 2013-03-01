#ifndef __GLUE_TICK_H__
#define __GLUE_TICK_H__

#ifdef LIBSPREFIX
#include <libs/types.h>
#else
#include <types.h>
#endif

#define tick_init     (*tick_init_ptr)
#define hpet_phys_get (*hpet_phys_get_ptr)

extern void tick_init(int freq);
extern uintptr_t hpet_phys_get(void);

#endif
