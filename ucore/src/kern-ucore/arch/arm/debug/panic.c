#include <types.h>
#include <kio.h>
#include <kdebug.h>
#include <stdio.h>
#include <intr.h>
#include <monitor.h>

static bool is_panic = 0;

/* *
 * __panic - __panic is called on unresolvable fatal errors. it prints
 * "panic: 'message'", and then enters the kernel monitor.
 * */
void __panic(const char *file, int line, const char *fmt, ...)
{
	if (is_panic) {
		goto panic_dead;
	}
	is_panic = 1;
	start_debug();

	// print the 'message'
	va_list ap;
	va_start(ap, fmt);
	kprintf("kernel panic at %s:%d:\n    ", file, line);
	vkprintf(fmt, ap);
	kprintf("\n");
	va_end(ap);

panic_dead:
	intr_disable();
	while (1) {
		monitor(NULL);
	}
}

/* __warn - like panic, but don't */
void __warn(const char *file, int line, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	kprintf("kernel warning at %s:%d:\n    ", file, line);
	vkprintf(fmt, ap);
	kprintf("\n");
	va_end(ap);
}

bool is_kernel_panic(void)
{
	return is_panic;
}
