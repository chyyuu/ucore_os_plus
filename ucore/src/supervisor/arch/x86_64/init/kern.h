#ifndef __KERN_H__
#define __KERN_H__

#include <mmu.h>

typedef struct kern_bootinfo_s {
	union {
		struct {
			uintptr_t kern_start;
			uintptr_t kern_entry;
			uintptr_t kern_text;
			uintptr_t kern_rodata;
			uintptr_t kern_pls;
			uintptr_t kern_data;
			uintptr_t kern_bss;
			uintptr_t kern_end;
		};

		char __space[PGSIZE];
	};
} kern_bootinfo_s;

extern kern_bootinfo_s kern_bootinfo;

void load_kern(void);
void jump_kern(void) __attribute__ ((noreturn));

#define EXPORT_SYMBOL(sym) char __TO_EXPORT_##sym[0] __attribute((unused));

#endif
