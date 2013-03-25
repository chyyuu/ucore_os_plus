#include <types.h>
#include <arch.h>
#include <error.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* *
 * Space or zero padding and a field width are supported for the numeric
 * formats only.
 *
 * The special format %e takes an integer error code
 * and prints a string describing the error.
 * The integer may be positive or negative,
 * so that -E_NO_MEM and E_NO_MEM are equivalent.
 * */

static const char *const error_string[MAXERROR + 1] = {
	[0] NULL,
	[E_PERM] "Operation not permitted",
	[E_NOENT] "No such file or directory",
	[E_SRCH] "No such process",
	[E_INTR] "Interrupted system call",
	[E_IO] "I/O error",
	[E_NXIO] "No such device or address",
	[E_2BIG] "Argument list too long",
	[E_NOEXEC] "Exec format error",
	[E_BADF] "Bad file number",
	[E_CHILD] "No child processes",
	[E_AGAIN] "Try again",
	[E_NOMEM] "Out of memory",
	[E_ACCES] "Permission denied",
	[E_FAULT] "Bad address",
	[E_NOTBLK] "Block device required",
	[E_BUSY] "Device or resource busy",
	[E_EXIST] "File exists",
	[E_XDEV] "Cross-device link",
	[E_NODEV] "No such device",
	[E_NOTDIR] "Not a directory",
	[E_ISDIR] "Is a directory",
	[E_INVAL] "Invalid argument",
	[E_NFILE] "File table overflow",
	[E_MFILE] "Too many open files",
	[E_NOTTY] "Not a typewriter",
	[E_TXTBSY] "Text file busy",
	[E_FBIG] "File too large",
	[E_NOSPC] "No space left on device",
	[E_SPIPE] "Illegal seek",
	[E_ROFS] "Read-only file system",
	[E_MLINK] "Too many links",
	[E_PIPE] "Broken pipe",
	[E_DOM] "Math argument out of domain of func",
	[E_RANGE] "Math result not representable",
	[E_UNIMP] "Unimplemented Feature",
	[E_PANIC] "Panic Failure",
	[E_KILLED] "Process is killed",
	[E_UNSPECIFIED] "Unspecified or unknown problem",
	[E_SWAP_FAULT] "SWAP READ/WRITE fault",
};

/* *
 * printnum - print a number (base <= 16) in reverse order
 * @putch:      specified putch function, print a single character
 * @fd:         file descriptor
 * @putdat:     used by @putch function
 * @num:        the number will be printed
 * @base:       base for print, must be in [1, 16]
 * @width:      maximum number of digits, if the actual width is less than @width, use @padc instead
 * @padc:       character that padded on the left if the actual width is less than @width
 * */
static void
printnum(void (*putch) (int, void *, int), int fd, void *putdat,
	 unsigned long long num, unsigned base, int width, int padc)
{
	unsigned long long result = num;
	unsigned mod = do_div(result, base);

	// first recursively print all preceding (more significant) digits
	if (num >= base) {
		printnum(putch, fd, putdat, result, base, width - 1, padc);
	} else {
		// print any needed pad characters before first digit
		while (--width > 0)
			putch(padc, putdat, fd);
	}
	// then print this (the least significant) digit
	putch("0123456789abcdef"[mod], putdat, fd);
}

#define GETINT_MACRO

/* *
 * getuint - get an unsigned int of various possible sizes from a varargs list
 * @ap:         a varargs list pointer
 * @lflag:      determines the size of the vararg that @ap points to
 * */
#ifdef GETINT_MACRO
#define getuint(ap, lflag)												\
	((lflag >= 2) ? (unsigned long long)va_arg(ap, unsigned long long) : \
	 (lflag)      ? (unsigned long long)va_arg(ap, unsigned long) :		\
	 (unsigned long long)va_arg(ap, unsigned int)						\
	)
#else
static unsigned long long getuint(va_list ap, int lflag)
{
	if (lflag >= 2) {
		return va_arg(ap, unsigned long long);
	} else if (lflag) {
		return va_arg(ap, unsigned long);
	} else {
		return va_arg(ap, unsigned int);
	}
}
#endif

/* *
 * getint - same as getuint but signed, we can't use getuint because of sign extension
 * @ap:         a varargs list pointer
 * @lflag:      determines the size of the vararg that @ap points to
 * */
#ifdef GETINT_MACRO
#define getint(ap, lflag)												\
	((lflag >= 2) ? (long long)va_arg(ap, long long) :					\
	 (lflag)      ? (long long)va_arg(ap, long) :						\
	 (long long)va_arg(ap, int)											\
	)
#else
static long long getint(va_list ap, int lflag)
{
	if (lflag >= 2) {
		return va_arg(ap, long long);
	} else if (lflag) {
		return va_arg(ap, long);
	} else {
		return va_arg(ap, int);
	}
}
#endif

/* *
 * printfmt - format a string and print it by using putch
 * @putch:      specified putch function, print a single character
 * @fd:         file descriptor
 * @putdat:     used by @putch function
 * @fmt:        the format string to use
 * */
void
printfmt(void (*putch) (int, void *, int), int fd, void *putdat,
	 const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintfmt(putch, fd, putdat, fmt, ap);
	va_end(ap);
}

/* *
 * vprintfmt - format a string and print it by using putch, it's called with a va_list
 * instead of a variable number of arguments
 * @putch:      specified putch function, print a single character
 * @fd:         file descriptor
 * @putdat:     used by @putch function
 * @fmt:        the format string to use
 * @ap:         arguments for the format string
 *
 * Call this function if you are already dealing with a va_list.
 * Or you probably want printfmt() instead.
 * */
void
vprintfmt(void (*putch) (int, void *, int), int fd, void *putdat,
	  const char *fmt, va_list ap)
{
	register const char *p;
	register int ch, err;
	unsigned long long num;
	int base, width, precision, lflag, altflag;

	while (1) {
		while ((ch = *(unsigned char *)fmt++) != '%') {
			if (ch == '\0') {
				return;
			}
			putch(ch, putdat, fd);
		}

		// Process a %-escape sequence
		char padc = ' ';
		width = precision = -1;
		lflag = altflag = 0;

reswitch:
		switch (ch = *(unsigned char *)fmt++) {

			// flag to pad on the right
		case '-':
			padc = '-';
			goto reswitch;

			// flag to pad with 0's instead of spaces
		case '0':
			padc = '0';
			goto reswitch;

			// width field
		case '1' ... '9':
			for (precision = 0;; ++fmt) {
				precision = precision * 10 + ch - '0';
				ch = *fmt;
				if (ch < '0' || ch > '9') {
					break;
				}
			}
			goto process_precision;

		case '*':
			precision = va_arg(ap, int);
			goto process_precision;

		case '.':
			if (width < 0)
				width = 0;
			goto reswitch;

		case '#':
			altflag = 1;
			goto reswitch;

process_precision:
			if (width < 0)
				width = precision, precision = -1;
			goto reswitch;

			// long flag (doubled for long long)
		case 'l':
			lflag++;
			goto reswitch;

			// character
		case 'c':
			putch(va_arg(ap, int), putdat, fd);
			break;

			// error message
		case 'e':
			err = va_arg(ap, int);
			if (err < 0) {
				err = -err;
			}
			if (err > MAXERROR || (p = error_string[err]) == NULL) {
				printfmt(putch, fd, putdat, "error %d", err);
			} else {
				printfmt(putch, fd, putdat, "%s", p);
			}
			break;

			// string
		case 's':
			if ((p = va_arg(ap, char *)) == NULL) {
				p = "(null)";
			}
			if (width > 0 && padc != '-') {
				for (width -= strnlen(p, precision); width > 0;
				     width--) {
					putch(padc, putdat, fd);
				}
			}
			for (;
			     (ch = *p++) != '\0' && (precision < 0
						     || --precision >= 0);
			     width--) {
				if (altflag && (ch < ' ' || ch > '~')) {
					putch('?', putdat, fd);
				} else {
					putch(ch, putdat, fd);
				}
			}
			for (; width > 0; width--) {
				putch(' ', putdat, fd);
			}
			break;

			// (signed) decimal
		case 'd':
			num = getint(ap, lflag);
			if ((long long)num < 0) {
				putch('-', putdat, fd);
				num = -(long long)num;
			}
			base = 10;
			goto number;

			// unsigned decimal
		case 'u':
			num = getuint(ap, lflag);
			base = 10;
			goto number;

			// (unsigned) octal
		case 'o':
			num = getuint(ap, lflag);
			base = 8;
			goto number;

			// pointer
		case 'p':
			putch('0', putdat, fd);
			putch('x', putdat, fd);
			num =
			    (unsigned long long)(uintptr_t) va_arg(ap, void *);
			base = 16;
			goto number;

			// (unsigned) hexadecimal
		case 'x':
		case 'X':
			num = getuint(ap, lflag);
			base = 16;
number:
			printnum(putch, fd, putdat, num, base, width, padc);
			break;

			// escaped '%' character
		case '%':
			putch(ch, putdat, fd);
			break;

			// unrecognized escape sequence - just print it literally
		default:
			putch('%', putdat, fd);
			for (fmt--; fmt[-1] != '%'; fmt--)
				/* do nothing */ ;
			break;
		}
	}
}

/* sprintbuf is used to save enough information of a buffer */
struct sprintbuf {
	char *buf;		// address pointer points to the first unused memory
	char *ebuf;		// points the end of the buffer
	int cnt;		// the number of characters that have been placed in this buffer
};

/* *
 * sprintputch - 'print' a single character in a buffer
 * @ch:         the character will be printed
 * @b:          the buffer to place the character @ch
 * */
static void sprintputch(int ch, struct sprintbuf *b)
{
	b->cnt++;
	if (b->buf < b->ebuf) {
		*b->buf++ = ch;
	}
}

/* *
 * vsnprintf - format a string and place it in a buffer, it's called with a va_list
 * instead of a variable number of arguments
 * @str:        the buffer to place the result into
 * @size:       the size of buffer, including the trailing null space
 * @fmt:        the format string to use
 * @ap:         arguments for the format string
 *
 * The return value is the number of characters which would be generated for the
 * given input, excluding the trailing '\0'.
 *
 * Call this function if you are already dealing with a va_list.
 * Or you probably want snprintf() instead.
 * */
static int ucore_vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
	struct sprintbuf b = { str, str + size - 1, 0 };
	if (str == NULL || b.buf > b.ebuf) {
		return -E_INVAL;
	}
	// print the string to the buffer
	vprintfmt((void *)sprintputch, NO_FD, &b, fmt, ap);
	// null terminate the buffer
	*b.buf = '\0';
	return b.cnt;
}

/* *
 * snprintf - format a string and place it in a buffer
 * @str:        the buffer to place the result into
 * @size:       the size of buffer, including the trailing null space
 * @fmt:        the format string to use
 * */
int snprintf(char *str, size_t size, const char *fmt, ...)
{
	va_list ap;
	int cnt;
	va_start(ap, fmt);
	cnt = ucore_vsnprintf(str, size, fmt, ap);
	va_end(ap);
	return cnt;
}
