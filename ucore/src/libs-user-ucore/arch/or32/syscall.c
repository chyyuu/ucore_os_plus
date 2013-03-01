#include <syscall.h>
#include <unistd.h>
#include <types.h>
#include <stdarg.h>

#define MAX_ARGS            5

uint32_t syscall(int num, ...)
{
	va_list ap;
	va_start(ap, num);
	uint32_t a[MAX_ARGS];
	int i;
	for (i = 0; i < MAX_ARGS; i++) {
		a[i] = va_arg(ap, uint32_t);
	}
	va_end(ap);

	uint32_t ret;
	register uint32_t _r3 asm("r3") = num;
	register uint32_t _r4 asm("r4") = a[0];
	register uint32_t _r5 asm("r5") = a[1];
	register uint32_t _r6 asm("r6") = a[2];
	register uint32_t _r7 asm("r7") = a[3];
	register uint32_t _r8 asm("r8") = a[4];
	asm volatile ("l.sys 1; l.nop;":"=r" (ret)
		      :"r"(_r3),
		      "r"(_r4), "r"(_r5), "r"(_r6), "r"(_r7), "r"(_r8));
	return ret;
}
