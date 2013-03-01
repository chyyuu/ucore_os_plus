#include <stdio.h>
#include <kio.h>
#include <stdarg.h>
#include <arch.h>

/**
 * Print the fatal error message and exit.
 */
void __panic(const char *file, const char *line, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	kprintf("kernel panic at %s:%d:\n    ", file, line);
	vkprintf(fmt, ap);
	kprintf("\n");
	va_end(ap);

	host_exit(SIGINT);
}

/**
 * Print a warning and continue.
 */
void __warn(const char *file, int line, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	kprintf("kernel warning at %s:%d:\n    ", file, line);
	vkprintf(fmt, ap);
	kprintf("\n");
	va_end(ap);
}
