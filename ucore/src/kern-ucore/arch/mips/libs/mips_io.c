#include <defs.h>
#include <arch.h>
#include <unistd.h>
#include <mips_io.h>
#include <stdio.h>
#include <console.h>

/* HIGH level console I/O */

/* *
 * cputch - writes a single character @c to stdout, and it will
 * increace the value of counter pointed by @cnt.
 * */
static void cputch(int c, int *cnt)
{
	cons_putc(c);
	(*cnt)++;
}

#if 0
/* *
 * kprintf - formats a string and writes it to stdout
 *
 * The return value is the number of characters which would be
 * written to stdout.
 * */
int kprintf(const char *str)
{
	int cnt = 0;
	while (*str) {
		kputchar(*str);
		cnt++;
		str++;
	}
	//va_start(ap, fmt);
	//cnt = vkprintf(fmt, ap);
	//va_end(ap);
	return cnt;
}
#endif

/* *
 * vkprintf - format a string and writes it to stdout
 *
 * The return value is the number of characters which would be
 * written to stdout.
 *
 * Call this function if you are already dealing with a va_list.
 * Or you probably want kprintf() instead.
 * */
int vkprintf(const char *fmt, va_list ap)
{
	int cnt = 0;
	vprintfmt((void *)cputch, NO_FD, &cnt, fmt, ap);
	return cnt;
}

/* *
 * kprintf - formats a string and writes it to stdout
 *
 * The return value is the number of characters which would be
 * written to stdout.
 * */
int kprintf(const char *fmt, ...)
{
	va_list ap;
	int cnt;
	va_start(ap, fmt);
	cnt = vkprintf(fmt, ap);
	va_end(ap);
	return cnt;
}

static const char *hexdigits = "0123456789ABCDEF";
void printhex(unsigned int x)
{
	char tmp[9];
	int i = 0;
	tmp[8] = 0;
	for (i = 7; i >= 0; i--) {
		tmp[i] = hexdigits[x & 0xf];
		x = x >> 4;
	}
	kprintf(tmp);
}

void printbase10(int x)
{
	unsigned int t;
	int i = 0;
	char buf[16];
	if (x < 0)
		kputchar('-');
	x = (x < 0) ? -x : x;
	while (x >= 10) {
		t = __divu10(x);
		buf[i++] = ('0' + (x - __mulu10(t)));
		x = t;
	}
	buf[i] = ('0' + x);
	for (; i >= 0; i--) {
		kputchar(buf[i]);
	}
}

/* kputchar - writes a single character to stdout */
void kputchar(int c)
{
	cons_putc(c);
}

/* *
 * kputs- writes the string pointed by @str to stdout and
 * appends a newline character.
 * */
int kputs(const char *str)
{
	int cnt = 0;
	char c;
	while ((c = *str++) != '\0') {
		cputch(c, &cnt);
	}
	cputch('\n', &cnt);
	return cnt;
}

/* getchar - reads a single non-zero character from stdin */
int getchar(void)
{
	int c;
	while ((c = cons_getc()) == 0)
		/* do nothing */ ;
	return c;
}
