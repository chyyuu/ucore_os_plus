/*
 * "Hello World" example.
 *
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example
 * designs. It runs with or without the MicroC/OS-II RTOS and requires a STDOUT
 * device in your system's hardware.
 * The memory footprint of this hosted application is ~69 kbytes by default
 * using the standard reference design.
 *
 * For a reduced footprint version of this template, and an explanation of how
 * to reduce the memory footprint for a given application, see the
 * "small_hello_world" template.
 *
 */

#include <stdio.h>
#include <system.h>
#include <assert.h>
#include <nios2.h>
#include <io.h>
#include <rf212.h>

#define RF212_BASE (RF212_0_BASE + 0xE0000000)

void rf212_int_handler()
{
	//kprintf("rf212_int_handler\n");
	uint32_t status;
	static int total = 0;
	static int next = 0;
	int count = 0;
	int len;
	while ((status = IORD(RF212_BASE, 0)) & 0x1) {//when recv_fifo is not empty
		len = IORD(RF212_BASE, 0x80);//length
		++total;
		char buf[8];
		int i;
		for (i = 0; i < 4; ++i)
			buf[i] = IORD(RF212_BASE, i + 0x81) & 0xFF;
		int* number = (int*)buf;
		int lqi = IORD(RF212_BASE, 1) & 0xFF;
		int ed = IORD(RF212_BASE, 2) & 0xFF;
		kprintf("recv tot=%d len=%d number=0x%x lqi=%02x ed=%02x\n", total, len, *number, lqi, ed);
		++count;

		IOWR(RF212_BASE, 0, 8);//DO RECV
	}
	/*
	if (total + ((len >> 8) & 0xFF) >= next) {
		next += 100;
		kprintf("recv %d packets this time! total=%d drop=%d\n", count, total, (len >> 8) & 0xFF);
	}
	*/
}

void rf212_init()
{
	kprintf("rf212_init!\n");
	uint32_t status, status_last = 0xFFFFFFFF;
	do {
		status = IORD(RF212_BASE, 0);
		if (status != status_last)
			kprintf("sr=%08x\n", status);
		status_last = status;
	} while (!(status & 0x4));
	
	alt_irq_enable(RF212_0_IRQ);
}

void rf212_send(uint8_t len, uint8_t *data)
{
	kprintf("rf212_send len=%d first=0x%x\n", len, data[0]);
	if (len > 125) {
		kprintf("rf212_send : len = %d must not be more than 125!!!\n");
		return;
	}
	uint32_t status, status_last = 0xFFFFFFFF;
	do {
		status = IORD(RF212_BASE, 0);
		//if (status_last != 0xFFFFFFFF && status != status_last)
		//	kprintf("status=%08x\n", status);
		status_last = status;
	} while (status & 0x2);//wait until send_fifo is not full
	int i;
	for (i = 0; i < len; ++i)
		IOWR(RF212_BASE, i + 0x81, data[i]);
	IOWR(RF212_BASE, 0x80, len + 2);
	IOWR(RF212_BASE, 0, 0x10);//DO SEND
}


