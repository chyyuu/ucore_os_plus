#ifndef __KERN_DRIVER_CLOCK_H__
#define __KERN_DRIVER_CLOCK_H__

#include <types.h>

extern volatile size_t ticks;

void clock_init_arm(uint32_t base, int irq);
//clear interrupt output after handling
void clock_clear(void);
void enable_timer_list();
void __common_timer_int_handler();

#endif /* !__KERN_DRIVER_CLOCK_H__ */
