		.text
		.global kernel_thread_entry
		// This should look like this in C:
		//     kernel_thread_entry (int (*fn)(void*), void *arg);
kernel_thread_entry:
		l.add	r31, r3, r0
		l.add	r3, r4, r0
		l.jalr	r31
		l.nop

		// the return value is stored in r11
		l.add	r3, r11, r0
		l.jal	do_exit
		l.nop

