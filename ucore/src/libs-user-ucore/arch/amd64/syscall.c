#include <types.h>
#include <stdarg.h>
#include <unistd.h>

#define MAX_ARGS            6

uint64_t syscall(int num, ...)
{
	va_list ap;
	va_start(ap, num);
	uint64_t a[MAX_ARGS];
	int i;
	for (i = 0; i < MAX_ARGS; i++) {
		a[i] = va_arg(ap, uint64_t);
	}
	va_end(ap);

	uint64_t ret;
	asm volatile ("movq 0x00(%%rbx), %%rdi;"
		      "movq 0x08(%%rbx), %%rsi;"
		      "movq 0x10(%%rbx), %%rdx;"
		      "movq 0x18(%%rbx), %%rcx;"
		      "movq 0x20(%%rbx), %%r8;"
		      "movq 0x28(%%rbx), %%r9;" "int %1":"=a" (ret)
		      :"i"(T_SYSCALL), "a"(num), "b"(a)
		      :"cc", "memory");
	return ret;
}
