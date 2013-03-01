#include <defs.h>
#include <unistd.h>
#include <stdarg.h>
#include <syscall.h>

#define MAX_ARGS            5
#define SYSCALL_BASE        0x80

uintptr_t syscall(int num, ...)
{
	va_list ap;
	va_start(ap, num);
	uint32_t arg[MAX_ARGS];
	int i, ret;
	for (i = 0; i < MAX_ARGS; i++) {
		arg[i] = va_arg(ap, uint32_t);
	}
	va_end(ap);

	//num += SYSCALL_BASE;
	asm volatile (".set noreorder;\n" "move $v0, %1;\n"	/* syscall no. */
		      "move $a0, %2;\n"
		      "move $a1, %3;\n"
		      "move $a2, %4;\n"
		      "move $a3, %5;\n"
		      "move $t0, %6;\n"
		      "syscall;\n" "nop;\n" "move %0, $v0;\n":"=r" (ret)
		      :"r"(num), "r"(arg[0]), "r"(arg[1]), "r"(arg[2]),
		      "r"(arg[3]), "r"(arg[4])
		      :"a0", "a1", "a2", "a3", "v0", "t0");
	return ret + 1 - 1;
}
