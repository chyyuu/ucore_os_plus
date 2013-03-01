/* drivers/input/keyboard/goldfish-events.c
**
** Copyright (C) 2007 Google, Inc.
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include <asm/irq.h>
#include <asm/io.h>

enum {
	REG_READ = 0x00,
	REG_SET_PAGE = 0x00,
	REG_LEN = 0x04,
	REG_DATA = 0x08,

	PAGE_NAME = 0x00000,
	PAGE_EVBITS = 0x10000,
	PAGE_ABSDATA = 0x20000 | EV_ABS,
};

struct event_dev {
	struct input_dev *input;
	int irq;
	void __iomem *addr;
	char name[0];
};

static irqreturn_t events_interrupt(int irq, void *dev_id)
{
	struct event_dev *edev = dev_id;
	unsigned type, code, value;

	type = __raw_readl(edev->addr + REG_READ);
	code = __raw_readl(edev->addr + REG_READ);
	value = __raw_readl(edev->addr + REG_READ);

//pr_debug("## events %08x %08x %08x\n", type, code ,value);
	input_event(edev->input, type, code, value);
	return IRQ_HANDLED;
}

static void events_import_bits(struct event_dev *edev, unsigned long bits[],
			       unsigned type, size_t count)
{
	int i, j;
	size_t size;
	uint8_t val;
	void __iomem *addr = edev->addr;
	__raw_writel(PAGE_EVBITS | type, addr + REG_SET_PAGE);
	size = __raw_readl(addr + REG_LEN) * 8;
	if (size < count)
		count = size;
	addr = addr + REG_DATA;
	for (i = 0; i < count; i += 8) {
		val = __raw_readb(addr++);
		for (j = 0; j < 8; j++)
			if (val & 1 << j)
				set_bit(i + j, bits);
	}
}

static int events_probe(struct platform_device *pdev)
{
	struct input_dev *input_dev;
	struct event_dev *edev = NULL;
	struct resource *res;
	unsigned keymapnamelen;
	int i;
	int count;
	int irq;
	void __iomem *addr;
	int ret;

	printk("*** events probe ***\n");

	input_dev = input_allocate_device();
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!input_dev || !res)
		goto fail;

	addr = ioremap(res->start, 4096);
	irq = platform_get_irq(pdev, 0);

	printk("events_probe() addr=0x%p irq=%d\n", addr, irq);

	if (!addr)
		goto fail;
	if (irq < 0)
		goto fail;

	__raw_writel(PAGE_NAME, addr + REG_SET_PAGE);
	keymapnamelen = __raw_readl(addr + REG_LEN);

	edev =
	    kzalloc(sizeof(struct event_dev) + keymapnamelen + 1, GFP_KERNEL);
	if (!edev)
		goto fail;

	edev->input = input_dev;
	edev->addr = addr;
	edev->irq = irq;

	for (i = 0; i < keymapnamelen; i++) {
		edev->name[i] = __raw_readb(edev->addr + REG_DATA + i);
	}
	printk("events_probe() keymap=%s\n", edev->name);

	events_import_bits(edev, input_dev->evbit, EV_SYN, EV_MAX);
	events_import_bits(edev, input_dev->keybit, EV_KEY, KEY_MAX);
	events_import_bits(edev, input_dev->relbit, EV_REL, REL_MAX);
	events_import_bits(edev, input_dev->absbit, EV_ABS, ABS_MAX);
	events_import_bits(edev, input_dev->mscbit, EV_MSC, MSC_MAX);
	events_import_bits(edev, input_dev->ledbit, EV_LED, LED_MAX);
	events_import_bits(edev, input_dev->sndbit, EV_SND, SND_MAX);
	events_import_bits(edev, input_dev->ffbit, EV_FF, FF_MAX);
	events_import_bits(edev, input_dev->swbit, EV_SW, SW_MAX);

	__raw_writel(PAGE_ABSDATA, addr + REG_SET_PAGE);
	count = __raw_readl(addr + REG_LEN) / (4 * 4);
	if (count > ABS_MAX)
		count = ABS_MAX;
	for (i = 0; i < count; i++) {
		int val[4];
		int j;
		if (!test_bit(i, input_dev->absbit))
			continue;
		for (j = 0; j < ARRAY_SIZE(val); j++)
			val[j] =
			    __raw_readl(edev->addr + REG_DATA +
					(i * ARRAY_SIZE(val) + j) * 4);
		input_set_abs_params(input_dev, i, val[0], val[1], val[2],
				     val[3]);
	}

	platform_set_drvdata(pdev, edev);

	input_dev->name = edev->name;
	input_set_drvdata(input_dev, edev);

	ret = input_register_device(input_dev);
	if (ret)
		goto fail;

	if (request_irq(edev->irq, events_interrupt, 0,
			"goldfish-events-keypad", edev) < 0) {
		input_unregister_device(input_dev);
		kfree(edev);
		return -EINVAL;
	}

	return 0;

fail:
	kfree(edev);
	input_free_device(input_dev);

	return -EINVAL;
}

static struct platform_driver events_driver = {
	.probe = events_probe,
	.driver = {
		   .name = "goldfish_events",
		   },
};

static int __devinit events_init(void)
{
	return platform_driver_register(&events_driver);
}

static void __exit events_exit(void)
{
}

module_init(events_init);
module_exit(events_exit);

MODULE_AUTHOR("Brian Swetland");
MODULE_DESCRIPTION("Goldfish Event Device");
MODULE_LICENSE("GPL");
