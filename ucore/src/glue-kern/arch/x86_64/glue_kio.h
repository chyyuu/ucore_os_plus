#ifndef __GLUE_KIO_H__
#define __GLUE_KIO_H__

#ifdef LIBSPREFIX
#include <libs/types.h>
#include <libs/stdarg.h>
#else
#include <types.h>
#include <stdarg.h>
#endif

#define kcons_putc (*kcons_putc_ptr)
#define kcons_getc (*kcons_getc_ptr)

extern void kcons_putc(int c);
extern int kcons_getc(void);

#endif
