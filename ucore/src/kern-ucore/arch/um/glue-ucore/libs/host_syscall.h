#ifndef __ARCH_UM_INCLUDE_HOST_SYSCALL_H__
#define __ARCH_UM_INCLUDE_HOST_SYSCALL_H__

#include <types.h>

struct proc_struct;

int host_mmap(struct proc_struct *proc,
	      void *addr, size_t length, int prot, int flags,
	      int fd, uint32_t offset);
int host_munmap(struct proc_struct *proc, void *addr, size_t length);
int host_assign(struct proc_struct *proc, uintptr_t addr, uint32_t data);
int host_getvalue(struct proc_struct *proc, uintptr_t addr, uint32_t * data);
int host_map_user(struct proc_struct *proc,
		  uintptr_t addr, size_t len, int is_write);

#endif /* !__ARCH_UM_INCLUDE_HOST_SYSCALL_H__ */
