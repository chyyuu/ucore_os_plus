.text
.globl kernel_thread_entry
kernel_thread_entry:        # void kernel_thread(void)
	# r0 already holds the arg
	
	# before call, set lr
	mov lr, pc
	add lr, lr, #4
  mov r0, r2
	# call fn (r1)
	mov pc, r1
	
	# save the return value of fn(arg) from r0 (return value) to r0 (arg of do_exit)
	#mov r0, r3
    # call do_exit to terminate current thread
	b do_exit

