.global mcount
.type mcount, @function

#define MAGIC_ECX 0xfeab0001

mcount:
	/* Allocate space for 7 registers.  */
	subq	$56,%rsp
	movq	%rax,(%rsp)
	movq	%rcx,8(%rsp)
	movq	%rdx,16(%rsp)
	movq	%rsi,24(%rsp)
	movq	%rdi,32(%rsp)
	movq	%r8,40(%rsp)
	movq	%r9,48(%rsp)

	/* Setup parameter for __mcount_internal.  */
	/* selfpc is the return address on the stack.  */
	movq	56(%rsp),%rsi
	/* Get frompc via the frame pointer.  */
	movq	8(%rbp),%rdi
	/* call something */
	mov     $MAGIC_ECX, %ecx
	wrmsr

	movq	48(%rsp),%r9
	movq	40(%rsp),%r8
	movq	32(%rsp),%rdi
	movq	24(%rsp),%rsi
	movq	16(%rsp),%rdx
	movq	8(%rsp),%rcx
	movq	(%rsp),%rax
	addq	$56,%rsp

	ret
