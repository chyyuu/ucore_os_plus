#ifndef __GLUE_CONTEXT_H__
#define __GLUE_CONTEXT_H__

#ifdef LIBSPREFIX
#include <libs/types.h>
#else
#include <types.h>
#endif

/* Symbols from supervisor */
typedef struct context_s *context_t;
typedef struct context_s {
	uintptr_t stk_ptr;
	uintptr_t stk_top;

	uintptr_t pc;
	int lcpu;
} context_s;

#define context_switch (*context_switch_ptr)
#define context_fill   (*context_fill_ptr)

extern void context_switch(context_t from, context_t to);
extern void context_fill(context_t ctx, void (*entry) (void *arg), void *arg,
			 uintptr_t stk_top);

#endif
