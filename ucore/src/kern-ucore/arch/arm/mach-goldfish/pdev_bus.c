/*
 * =====================================================================================
 *
 *       Filename:  pdev_bus.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  07/04/2012 02:45:49 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <board.h>
#include <arm.h>
#include <stdio.h>
#include <pdev_bus.h>
#include <serial.h>
#include <clock.h>
#include <string.h>

struct pdev_device {
	uint32_t id, base, size, irq, irq_count, name_len;
	uint32_t state;
	char name[64];
};

static int pdev_device_init(struct pdev_device *dev)
{
	// qemu serial
	if (!strcmp(dev->name, "goldfish_tty") && dev->id == 2) {
		extern void serial_init(uint32_t base, uint32_t irq);
		serial_init(dev->base, dev->irq);
	} else if (!strcmp(dev->name, "goldfish_fb")) {
		//fb_init(dev->base, dev->irq);
	}
}

void pdev_bus_init()
{
	struct pdev_device device;
	uint32_t state;
	kprintf("Probing devices on pdev bus...\n");
	outw(GOLDFISH_PDEV_BUS + PDEV_BUS_OP, PDEV_BUS_OP_INIT);

	while (1) {
		state = inw(GOLDFISH_PDEV_BUS + PDEV_BUS_OP);
		if (state == PDEV_BUS_OP_DONE)
			break;
		device.id = inw(GOLDFISH_PDEV_BUS + PDEV_BUS_ID);
		device.base = inw(GOLDFISH_PDEV_BUS + PDEV_BUS_IO_BASE);
		device.size = inw(GOLDFISH_PDEV_BUS + PDEV_BUS_IO_SIZE);
		device.irq = inw(GOLDFISH_PDEV_BUS + PDEV_BUS_IRQ);
		device.irq_count = inw(GOLDFISH_PDEV_BUS + PDEV_BUS_IRQ_COUNT);
		device.name_len = inw(GOLDFISH_PDEV_BUS + PDEV_BUS_NAME_LEN);
		device.name[0] = 0;
		if (device.name_len < 61) {
			outw(GOLDFISH_PDEV_BUS + PDEV_BUS_GET_NAME,
			     (uint32_t) device.name);
			device.name[device.name_len] = 0;
		}
		kprintf
		    ("  Dev: %d, %s, base=0x%08x, size=0x%x, irq=%d, irq_count=%d\n",
		     device.id, device.name[0] ? device.name : "<TOO LONG>",
		     device.base, device.size, device.irq, device.irq_count);
		pdev_device_init(&device);
	}

}
