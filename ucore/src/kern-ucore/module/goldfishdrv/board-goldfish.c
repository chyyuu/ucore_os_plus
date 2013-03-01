/* arch/arm/mach-goldfish/board-goldfish.c
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
//#include <linux/mtd/mtd.h>
//#include <linux/mtd/nand.h>
//#include <linux/mtd/partitions.h>
#include <linux/input.h>

#include <mach/hardware.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/map.h>
#include <asm/mach/time.h>

int GOLDFISH_READY = 0;

static struct resource goldfish_pdev_bus_resources[] = {
	{
	 .start = GOLDFISH_PDEV_BUS_BASE,
	 .end = GOLDFISH_PDEV_BUS_BASE + GOLDFISH_PDEV_BUS_END - 1,
	 .flags = IORESOURCE_IO,
	 },
	{
	 .start = IRQ_PDEV_BUS,
	 .end = IRQ_PDEV_BUS,
	 .flags = IORESOURCE_IRQ,
	 }
};

struct platform_device goldfish_pdev_bus_device = {
	.name = "goldfish_pdev_bus",
	.id = -1,
	.num_resources = ARRAY_SIZE(goldfish_pdev_bus_resources),
	.resource = goldfish_pdev_bus_resources
};

static void __init goldfish_init(void)
{
	platform_device_register(&goldfish_pdev_bus_device);
}

void goldfish_mask_irq(unsigned int irq)
{
	writel(irq,
	       IO_ADDRESS(GOLDFISH_INTERRUPT_BASE) +
	       GOLDFISH_INTERRUPT_DISABLE);
}

void goldfish_unmask_irq(unsigned int irq)
{
	writel(irq,
	       IO_ADDRESS(GOLDFISH_INTERRUPT_BASE) + GOLDFISH_INTERRUPT_ENABLE);
}

static struct irq_chip goldfish_irq_chip = {
	.name = "goldfish",
	.mask = goldfish_mask_irq,
	.mask_ack = goldfish_mask_irq,
	.unmask = goldfish_unmask_irq,
};

void goldfish_init_irq(void)
{
	unsigned int i;
	uint32_t int_base = IO_ADDRESS(GOLDFISH_INTERRUPT_BASE);

	/*
	 * Disable all interrupt sources
	 */
	writel(1, int_base + GOLDFISH_INTERRUPT_DISABLE_ALL);

#if 0
	for (i = 0; i < NR_IRQS; i++) {
		set_irq_chip(i, &goldfish_irq_chip);
		set_irq_handler(i, handle_level_irq);
		set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
	}
#endif
}

static struct map_desc goldfish_io_desc[] __initdata = {
	{
	 .virtual = IO_BASE,
	 .pfn = __phys_to_pfn(IO_START),
	 .length = IO_SIZE,
	 .type = MT_DEVICE},
};

static void __init goldfish_map_io(void)
{
	iotable_init(goldfish_io_desc, ARRAY_SIZE(goldfish_io_desc));
	GOLDFISH_READY = 1;
}

//extern struct sys_timer goldfish_timer;

MACHINE_START(GOLDFISH, "Goldfish")
    .phys_io = IO_START,.io_pg_offst = ((IO_BASE) >> 18) & 0xfffc,.boot_params =
    0x00000100,.map_io = goldfish_map_io,.init_irq =
    goldfish_init_irq,.init_machine = goldfish_init,
    //.timer                = &goldfish_timer,
    MACHINE_END
