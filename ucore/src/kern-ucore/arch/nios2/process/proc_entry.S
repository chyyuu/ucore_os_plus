.text
.globl kernel_thread_entry
kernel_thread_entry:        # void kernel_thread(void)

#    pushl %edx              # push arg
#    call *%ebx              # call fn
    callr r3

    mov r4, r2              # save the return value of fn(arg)
    call do_exit            # call do_exit to terminate current thread

