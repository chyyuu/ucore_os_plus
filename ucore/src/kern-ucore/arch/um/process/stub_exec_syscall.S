#include <memlayout.h>

		.section .__syscall_stub, "ax"

		.globl stub_exec_syscall
stub_exec_syscall:

		movl STUB_DATA, %esp
		movl 0(%esp), %eax
		cmp  $0, %eax
		jz	 read
		cmp  $-1, %eax
		jz	 write
		movl 4(%esp), %ebx
		movl 8(%esp), %ecx
		movl 12(%esp), %edx
		movl 16(%esp), %esi
		movl 20(%esp), %edi

		int $0x80

		jmp exit

read:
		# PTRACE_PEEKDATA doesn't work actually.
		movl 4(%esp), %ebx
		movl (%ebx), %eax
		jmp exit

write:
		movl 4(%esp), %ebx
		movl 8(%esp), %ecx
		movl %ecx, (%ebx)
		movl $0, %eax

exit:	
		movl %eax, 0(%esp)

		int3