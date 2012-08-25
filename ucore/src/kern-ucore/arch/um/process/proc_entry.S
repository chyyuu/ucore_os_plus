.text
.globl kernel_thread_entry
kernel_thread_entry:        # void kernel_thread(void)

    pushl %ebp              # push arg
    call *%ebx              # call fn

    pushl %eax              # save the return value of fn(arg)
    call do_exit            # call do_exit to terminate current thread