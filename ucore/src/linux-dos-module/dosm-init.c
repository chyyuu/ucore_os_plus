#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/ioport.h>
#include <linux/signal.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <asm/irq.h>
#include <asm/io.h>

#define __DOSM__
#include "dosm-packet.h"

/* Static assert */
static char __sa[sizeof(dosm_packet_s) == DOSM_PACKET_SIZE ? 0 : -1]
    __attribute__ ((unused));

static uintptr_t buf_paddr;
static ulong buf_size;
ushort ipi_vector;
EXPORT_SYMBOL(ipi_vector);

/* Should pass the size check */
module_param(buf_paddr, ulong, 0);
module_param(buf_size, ulong, 0);
module_param(ipi_vector, ushort, 0);

static struct task_struct *dosm_task;
static void (*original_ipi_callback) (void);
static struct resource *buf_res;
static volatile char *buf;

static volatile dosm_packet_t packet_buf;
static int packet_buf_cap;

static struct semaphore scan_sem;
static struct timer_list scan_timer;

static int dosm_kthread(void *arg)
{
	int i;

	allow_signal(SIGKILL);
	set_current_state(TASK_INTERRUPTIBLE);

	while (!kthread_should_stop()) {
		if (down_interruptible(&scan_sem) != 0)
			continue;

		for (i = 0; i < packet_buf_cap; ++i) {
			if ((packet_buf[i].source_flags & DOSM_SF_VALID) &&
			    !(packet_buf[i].remote_flags & DOSM_RF_CHECKED)) {
				packet_buf[i].remote_flags |= DOSM_RF_CHECKED;
				printk(KERN_ALERT
				       "processing packet %d from %d\n", i,
				       packet_buf[i].source_lapic);
				dosm_packet_process(packet_buf + i);
			}
		}
	}

	return 0;
}

static void dosm_scan_timer(ulong arg)
{
	/* We do not use the result, with a trick */
	int res;
	res = down_trylock(&scan_sem);
	up(&scan_sem);

	scan_timer.expires = jiffies + HZ;
	add_timer(&scan_timer);
}

static void dosm_ipi_callback(void)
{
	/* We do not use the result, with a trick */
	int res;
	res = down_trylock(&scan_sem);
	up(&scan_sem);

	if (original_ipi_callback)
		original_ipi_callback();
}

static int __init dosm_init(void)
{
	printk(KERN_INFO "UCORE-MP64 DOS Module, buf=%p size=%lu ipi=%d.\n",
	       (void *)buf_paddr, buf_size, ipi_vector);

	packet_buf_cap = buf_size / DOSM_PACKET_SIZE;

	/* Init the sem */
	sema_init(&scan_sem, 0);

	/* Setup a kernel thread for receiving request and executing them */
	kthread_run(dosm_kthread, NULL, "DOSM Thread");

	/* Set the io mem region */
	buf_res = request_mem_region(buf_paddr, buf_size, "DOSM IO BUF");
	buf = ioremap(buf_paddr, buf_size);

	packet_buf = (dosm_packet_t) buf;

	/* Hook the ipi callback */
	original_ipi_callback = x86_platform_ipi_callback;
	x86_platform_ipi_callback = dosm_ipi_callback;

	/* Setting the timer */
	init_timer(&scan_timer);
	scan_timer.expires = jiffies + HZ;
	scan_timer.function = dosm_scan_timer;
	add_timer(&scan_timer);
	return 0;
}

static void __exit dosm_exit(void)
{
	kthread_stop(dosm_task);
	iounmap(buf);
	release_mem_region(buf_paddr, buf_size);
	printk(KERN_INFO "UCORE-MP64 Driver OS Module exits.\n");
}

module_init(dosm_init);
module_exit(dosm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("xinhaoyuan@gmail.com ");
MODULE_DESCRIPTION("Linux as a driver os for ucore-mp64");
