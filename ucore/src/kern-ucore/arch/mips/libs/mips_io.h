#ifndef __LIBS_MIPS_IO_H__
#define __LIBS_MIPS_IO_H__

#include <defs.h>
#include <stdarg.h>

/* kern/libs/stdio.c */
int vkprintf(const char *fmt, va_list ap);
int kprintf(const char *fmt, ...);
void kputchar(int c);
int kputs(const char *str);
int getchar(void);

void printhex(unsigned int x);
void printbase10(int x);

/* kern/libs/readline.c */
char *readline(const char *prompt);

#define PRINT_HEX(str,x) {kprintf(str);printhex((unsigned int)x);kprintf("\n");}

#endif /* !__LIBS_STDIO_H__ */
