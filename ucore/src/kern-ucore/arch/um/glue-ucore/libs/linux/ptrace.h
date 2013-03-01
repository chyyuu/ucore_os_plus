#ifndef __ARCH_UM_INCLUDE_LINUX_PTRACE_H__
#define __ARCH_UM_INCLUDE_LINUX_PTRACE_H__

#define PTRACE_TRACEME		   0
#define PTRACE_PEEKTEXT		   1
#define PTRACE_PEEKDATA		   2
#define PTRACE_PEEKUSR		   3
#define PTRACE_POKETEXT		   4
#define PTRACE_POKEDATA		   5
#define PTRACE_POKEUSR		   6
#define PTRACE_CONT		       7
#define PTRACE_KILL		       8
#define PTRACE_SINGLESTEP	   9
#define PTRACE_GETREGS        12
#define PTRACE_SETREGS        13
#define PTRACE_GETFPREGS      14
#define PTRACE_SETFPREGS      15
#define PTRACE_ATTACH		  16
#define PTRACE_DETACH		  17
#define PTRACE_OLDSETOPTIONS  21
#define PTRACE_SYSCALL		  24
#define PTRACE_SYSEMU		  31
#define PTRACE_SYSEMU_SINGLESTEP  32

/* 0x4200-0x4300 are reserved for architecture-independent additions.  */
#define PTRACE_SETOPTIONS	0x4200
#define PTRACE_GETEVENTMSG	0x4201
#define PTRACE_GETSIGINFO	0x4202
#define PTRACE_SETSIGINFO	0x4203

enum __ptrace_setoptions {
	PTRACE_O_TRACESYSGOOD = 0x00000001,
	PTRACE_O_TRACEFORK = 0x00000002,
	PTRACE_O_TRACEVFORK = 0x00000004,
	PTRACE_O_TRACECLONE = 0x00000008,
	PTRACE_O_TRACEEXEC = 0x00000010,
	PTRACE_O_TRACEVFORKDONE = 0x00000020,
	PTRACE_O_TRACEEXIT = 0x00000040,
	PTRACE_O_MASK = 0x0000007f
};

/* copied from user.h */
struct user_regs_struct {
	long int ebx;
	long int ecx;
	long int edx;
	long int esi;
	long int edi;
	long int ebp;
	long int eax;
	long int xds;
	long int xes;
	long int xfs;
	long int xgs;
	long int orig_eax;
	long int eip;
	long int xcs;
	long int eflags;
	long int esp;
	long int xss;
};

#define FAULT_P   0x1
#define FAULT_W   0x2

struct faultinfo {
	int error_code;		/* in ptrace_faultinfo misleadingly called is_write */
	unsigned long cr2;	/* in ptrace_faultinfo called addr */
	int trap_no;		/* missing in ptrace_faultinfo */
};

struct um_pt_regs {
	struct user_regs_struct regs;
	struct faultinfo faultinfo;
	long syscall;
	int is_user;
};

#endif /* !__ARCH_UM_INCLUDE_LINUX_PTRACE_H__ */
