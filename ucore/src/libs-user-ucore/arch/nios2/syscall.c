#include <unistd.h>
#include <types.h>
#include <stdarg.h>
#include <syscall.h>
#include <mboxbuf.h>
#include <stat.h>
#include <dirent.h>

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

	register uint32_t __res __asm__("r2");
	register uint32_t __sc __asm__("r4") = (uint32_t) num;
	register uint32_t __a __asm__("r5") = (uint32_t) a[0];
	register uint32_t __b __asm__("r6") = (uint32_t) a[1];
	register uint32_t __c __asm__("r7") = (uint32_t) a[2];
	register uint32_t __d __asm__("r8") = (uint32_t) a[3];
	register uint32_t __e __asm__("r9") = (uint32_t) a[4];

	__asm__ __volatile__("trap":"=r"(__res)
			     :"0"(__sc), "r"(__a), "r"(__b), "r"(__c), "r"(__d),
			     "r"(__e)
			     :"memory");

	return __res;
}
