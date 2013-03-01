/*
 * Access system mailboxes
 */
#include "mailbox.h"
#include "barrier.h"

/* Mailbox memory addresses */
static volatile unsigned int *MAILBOX0READ = (unsigned int *)0x2000b880;
static volatile unsigned int *MAILBOX0STATUS = (unsigned int *)0x2000b898;
static volatile unsigned int *MAILBOX0WRITE = (unsigned int *)0x2000b8a0;

/* Bit 31 set in status register if the write mailbox is full */
#define MAILBOX_FULL 0x80000000

/* Bit 30 set in status register if the read mailbox is empty */
#define MAILBOX_EMPTY 0x40000000

unsigned int readmailbox(unsigned int channel)
{
	unsigned int count = 0;
	unsigned int data;

	/* Loop until something is received from channel
	 * If nothing recieved, it eventually give up and returns 0xffffffff
	 */
	while (1) {
		while (*MAILBOX0STATUS & MAILBOX_EMPTY) {
			/* Need to check if this is the right thing to do */
			flushcache();

			/* This is an arbritarily large number */
			if (count++ > (1 << 25)) {
				return 0xffffffff;
			}
		}
		/* Read the data
		 * Data memory barriers as we've switched peripheral
		 */
		dmb();
		data = *MAILBOX0READ;
		dmb();

		if ((data & 15) == channel)
			return data;
	}
}

void writemailbox(unsigned int channel, unsigned int data)
{
	/* Wait for mailbox to be not full */
	while (*MAILBOX0STATUS & MAILBOX_FULL) {
		/* Need to check if this is the right thing to do */
		flushcache();
	}

	dmb();
	*MAILBOX0WRITE = (data | channel);
}
