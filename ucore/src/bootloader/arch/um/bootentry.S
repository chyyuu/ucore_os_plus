#include <glue_memlayout.h>
		
		.section .__boot, "ax"

		.globl boot_main
		.globl stk

		.globl __boot_entry
__boot_entry:
		movl	stk, %esp
		
		movl	$91, %eax
		movl	$(USERBASE), %ebx
		movl	$(USERTOP - USERBASE), %ecx
		int 	$0x80

		movl 	$91, %eax
		movl	$(RESERVED_END), %ebx
		movl	$(HOSTBASE - RESERVED_END), %ecx
		int		$0x80

		movl 	$90, %eax
		movl	%esp, %ebx
		int		$0x80

		movl	$(RESERVED_END - 0x1000), %esp

		call 	boot_main
