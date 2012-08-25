#ifndef __GLUE_UCORE_KIO_H__
#define __GLUE_UCORE_KIO_H__

#include <types.h>
#include <stdarg.h>
#include <glue_kio.h>

int kprintf(const char *fmt, ...);
int vkprintf(const char *fmt, va_list ap);

/* libs/readline.c */
char *readline(const char *prompt);

#endif
