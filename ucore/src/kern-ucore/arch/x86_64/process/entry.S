.text
.globl kernel_thread_entry
kernel_thread_entry:        # void kernel_thread(void)

	# kern_enter(0)
#if 0
	push %rdi
	push %rsi
	movq $0x0, %rdi
	call kern_enter
	pop  %rsi
	pop  %rdi
#endif
	
    movq %rdi, %rax         # mov fn to rax
    movq %rsi, %rdi         # prepare arg
    callq *%rax             # call fn

    movq %rax, %rdi         # save the return value of fn(arg)
    call do_exit            # call do_exit to terminate current thread

