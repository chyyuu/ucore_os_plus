/*  Kernel Programming */
#include <linux/module.h>	// Needed by all modules
#include <linux/kernel.h>	// Needed for KERN_ALERT
#include <linux/init.h>		// Needed for the macros
#include <linux/device.h>
#include <linux/kobject.h>

struct test_device_driver {
	char *name;
	struct device_driver driver;
};

static struct test_device_driver test_driver = {
	.name = "test_driver",
};

static int test_bus_match(struct device *dev, struct device_driver *drv)
{
	return 1;
}

struct bus_type test_bus_type = {
	.name = "test_bus",
	.match = test_bus_match,
};

static int __init test_bus_init()
{
	printk(KERN_ALERT "bus register");
	return bus_register(&test_bus_type);
}

static int test_driver_register(struct test_device_driver *test_driver)
{
	test_driver->driver.name = test_driver->name;
	test_driver->driver.bus = &test_bus_type;
	return driver_register(&test_driver->driver);
}

static int hello_init(void)
{
	int ret = -1;
	printk(KERN_ALERT "Hello, world\n");
	ret = test_bus_init();
	if (ret)
		printk(KERN_ALERT "REG ERR\n");
	else
		printk(KERN_ALERT "REG OK\n");
	ret = test_driver_register(&test_driver);
	if (ret)
		printk(KERN_ALERT "REG ERR\n");
	else
		printk(KERN_ALERT "REG OK\n");
	return 0;
}

static void hello_exit(void)
{
	printk(KERN_ALERT "Goodbye, world\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
