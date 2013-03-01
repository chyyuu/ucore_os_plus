/*
 * =====================================================================================
 *
 *       Filename:  irq_glue.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/21/2012 11:26:19 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>

#define __UCORE_NO_TYPE__
#include <picirq.h>

/* irq */
int request_irq(unsigned int irq, irq_handler_t handler,
		unsigned long irqflags, const char *devname, void *dev_id)
{
	if (irq > 31)
		return -EINVAL;
	printk(KERN_DEBUG "request_irq %d\n", irq);
	register_irq(irq, handler, dev_id);
	pic_enable(irq);
	return 0;
}

void free_irq(unsigned int irq, void *dev_id)
{
	printk(KERN_ALERT "TODO %s\n", __func__);
	pic_disable(irq);
	return;
}
