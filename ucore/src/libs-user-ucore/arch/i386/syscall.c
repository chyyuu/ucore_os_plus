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

	uint32_t ret;
	asm volatile ("int %1;":"=a" (ret)
		      :"i"(T_SYSCALL),
		      "a"(num),
		      "d"(a[0]), "c"(a[1]), "b"(a[2]), "D"(a[3]), "S"(a[4])
		      :"cc", "memory");
	return ret;
}
