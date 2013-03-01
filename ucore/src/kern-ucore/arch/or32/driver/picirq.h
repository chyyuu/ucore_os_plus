#ifndef __KERN_DRIVER_PICIRQ_H__
#define __KERN_DRIVER_PICIRQ_H__

void pic_init(void);
void pic_enable(unsigned int irq);

/*! TODO: Define the value according to the arch. */
#define IRQ_UART       2
#define IRQ_TIMER      3
#define IRQ_OFFSET     32

#endif /* !__KERN_DRIVER_PICIRQ_H__ */
