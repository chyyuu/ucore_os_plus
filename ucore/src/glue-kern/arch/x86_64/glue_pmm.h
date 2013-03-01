#ifndef __GLUE_PMM_H__
#define __GLUE_PMM_H__

#ifdef LIBSPREFIX
#include <libs/types.h>
#else
#include <types.h>
#endif

#include "glue_memlayout.h"

#define TEST_PAGE         0x0

struct segdesc;

#define kalloc_pages      (*kalloc_pages_ptr)
#define kfree_pages       (*kfree_pages_ptr)
#define kpage_private_set (*kpage_private_set_ptr)
#define kpage_private_get (*kpage_private_get_ptr)
#define load_rsp0         (*load_rsp0_ptr)
#define init_pgdir_get    (*init_pgdir_get_ptr)
#define print_pgdir       (*print_pgdir_ptr)
#define get_sv_gdt		  (*get_sv_gdt_ptr)

extern uintptr_t kalloc_pages(size_t npages);
extern void kfree_pages(uintptr_t basepa, size_t npages);
extern void kpage_private_set(uintptr_t pa, void *private);
extern void *kpage_private_get(uintptr_t pa);
extern void load_rsp0(uintptr_t rsp0);
extern pgd_t *init_pgdir_get(void);
extern void print_pgdir(int (*printf) (const char *fmt, ...));
extern struct segdesc *get_sv_gdt();

/* Simply translate between VA and PA without checking */
#define KADDR_DIRECT(addr) ((void*)((uintptr_t)(addr) + PBASE))
#define PADDR_DIRECT(addr) ((uintptr_t)(addr) - PBASE)

#endif
