#ifndef __LIBS_STDARG_H__
#define __LIBS_STDARG_H__

/* compiler provides size of save area */
typedef __builtin_va_list va_list;

#define va_start(ap, last)              (__builtin_va_start(ap, last))
#define va_arg(ap, type)                (__builtin_va_arg(ap, type))
#define va_end(ap)		/*nothing */

#ifndef va_copy
#ifdef __va_copy
#define va_copy(DEST,SRC) __va_copy((DEST),(SRC))
#else
#define va_copy(DEST, SRC) memcpy((&DEST), (&SRC), sizeof(va_list))
#endif
#endif

#endif /* !__LIBS_STDARG_H__ */
