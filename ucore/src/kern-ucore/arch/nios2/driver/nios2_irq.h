#ifndef __NIOS2_KERN_DRIVER_IRQ_H__
#define __NIOS2_KERN_DRIVER_IRQ_H__

#include <types.h>

void irq_init(void);

inline int alt_irq_disable(uint32_t id);
inline int alt_irq_enable(uint32_t id);

#define IRQ_OFFSET		32

#endif /* !__NIOS2_KERN_DRIVER_IRQ_H__ */
