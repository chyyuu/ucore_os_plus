#ifndef __ARCH_UM_INCLUDE_LINUX_SETJMP_H__
#define __ARCH_UM_INCLUDE_LINUX_SETJMP_H__

#ifndef __ASSEMBLER__

#include <types.h>

struct __jmp_buf {
	uint32_t __ebx;
	uint32_t __esp;
	uint32_t __ebp;
	uint32_t __esi;
	uint32_t __edi;
	uint32_t __eip;
};

typedef struct __jmp_buf jmp_buf[1];

int setjmp(jmp_buf);
void longjmp(jmp_buf, int);

#endif /* !__ASSEMBLER__ */

#endif /* !__ARCH_UM_INCLUDE_LINUX_SETJMP_H__ */
