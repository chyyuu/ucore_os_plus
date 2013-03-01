#ifndef __ARCH_UM_BOOT_BOOT_H__
#define __ARCH_UM_BOOT_BOOT_H__

extern char __boot_start[];
void __boot_entry(void);

#define __pa(addr) (void*)((int)(addr) - (int)__boot_start + BOOT_CODE)

#define ROUNDUP_PAGE(addr)						\
	((addr + (PGSIZE-1)) & ~(PGSIZE-1))

/* used to keep data when the space is flushed */
struct temp_stack {
	int addr;
	int length;
	int prot;
	int flags;
	int fd;
	int offset;
};

#endif /* !__ARCH_UM_BOOT_BOOT_H__ */
