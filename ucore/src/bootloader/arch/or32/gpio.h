#ifndef __ARCH_OR32_DRIVERS_GPIO_H__
#define __ARCH_OR32_DRIVERS_GPIO_H__

#include <board.h>

#define GPIO_BASE   (0xf0000000 + (GPIO_PHYSICAL_BASE >> 4))

#define GPIO_INPUT          0x0
#define GPIO_OUTPUT         0x4

#endif /* !__ARCH_OR32_DRIVERS_GPIO_H__ */
