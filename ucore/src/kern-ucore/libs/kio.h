#ifndef __GLUE_UCORE_KIO_H__
#define __GLUE_UCORE_KIO_H__

#include <types.h>
#include <stdarg.h>

void cons_putc(int c);
int cons_getc(void);

int kprintf(const char *fmt, ...);
int vkprintf(const char *fmt, va_list ap);

/* libs/readline.c */
char *readline(const char *prompt);

#endif
