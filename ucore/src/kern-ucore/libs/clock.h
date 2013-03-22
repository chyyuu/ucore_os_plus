#ifndef __GLUE_UCORE_CLOCK_H__
#define __GLUE_UCORE_CLOCK_H__

#include <types.h>

extern volatile size_t ticks;
void clock_init(void);
void clock_init_ap(void);

#endif
