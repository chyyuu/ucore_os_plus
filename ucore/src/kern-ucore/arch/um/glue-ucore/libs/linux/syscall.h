#ifndef __ARCH_UM_INCLUDE_LINUX_SYSCALL_H__
#define __ARCH_UM_INCLUDE_LINUX_SYSCALL_H__

#define __NR_exit        1
#define __NR_fork        2
#define __NR_read        3
#define __NR_write       4
#define __NR_open        5
#define __NR_close       6
#define __NR_waitpid     7
#define __NR_unlink      10
#define __NR_lseek       19
#define __NR_getpid      20
#define __NR_ptrace      26
#define __NR_kill        37
#define __NR_sys_signal  48
#define __NR_ioctl       54
#define __NR_getppid     64
#define __NR_sigaction   67
#define __NR_sigpending  73
#define __NR_mmap		 90
#define __NR_munmap      91
#define __NR_setitimer   104
#define __NR_sigprocmask 126
#define __NR_nanosleep   162
#define __NR_sigaltstack 186

#ifndef __ASSEMBLER__

/* structs for syscalls with more than or equal six arguments. */
struct mmap_arg_struct {
	unsigned long addr;
	unsigned long len;
	unsigned long prot;
	unsigned long flags;
	unsigned long fd;
	unsigned long offset;
};

static inline long syscall0(long syscall) __attribute__ ((always_inline));
static inline long syscall1(long syscall, long arg1)
    __attribute__ ((always_inline));
static inline long syscall2(long syscall, long arg1, long arg2)
    __attribute__ ((always_inline));
static inline long syscall3(long syscall, long arg1, long arg2, long arg3)
    __attribute__ ((always_inline));
static inline long syscall4(long syscall, long arg1, long arg2, long arg3,
			    long arg4) __attribute__ ((always_inline));
static inline long syscall5(long syscall, long arg1, long arg2, long arg3,
			    long arg4, long arg5)
    __attribute__ ((always_inline));
static inline void trap_myself(void) __attribute__ ((always_inline));

/* syscall wrappers. copied from arch/um/sys-i386/shared/sysdep/stub.h */
static inline long syscall0(long syscall)
{
	long ret;
	__asm__ volatile ("int $0x80":"=a" (ret):"0"(syscall));
	return ret;
}

static inline long syscall1(long syscall, long arg1)
{
	long ret;
	__asm__ volatile ("int $0x80":"=a" (ret):"0"(syscall), "b"(arg1));
	return ret;
}

static inline long syscall2(long syscall, long arg1, long arg2)
{
	long ret;
	__asm__ volatile ("int $0x80":"=a" (ret):"0"(syscall), "b"(arg1),
			  "c"(arg2));
	return ret;
}

static inline long syscall3(long syscall, long arg1, long arg2, long arg3)
{
	long ret;
	__asm__ volatile ("int $0x80":"=a" (ret):"0"(syscall), "b"(arg1),
			  "c"(arg2), "d"(arg3));
	return ret;
}

static inline long syscall4(long syscall, long arg1, long arg2, long arg3,
			    long arg4)
{
	long ret;
	__asm__ volatile ("int $0x80":"=a" (ret):"0"(syscall), "b"(arg1),
			  "c"(arg2), "d"(arg3), "S"(arg4));
	return ret;
}

static inline long syscall5(long syscall, long arg1, long arg2, long arg3,
			    long arg4, long arg5)
{
	long ret;
	__asm__ volatile ("int $0x80":"=a" (ret):"0"(syscall), "b"(arg1),
			  "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5));
	return ret;
}

static inline void trap_myself(void)
{
	__asm("int3");
}

static inline void pause(void)
{
	asm("movl $29, %eax; int $0x80;");
}

#endif /* !__ASSEMBLER__ */

#endif /* !__ARCH_UM_INCLUDE_LINUX_SYSCALL_H__ */
