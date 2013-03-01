#ifndef __ARCH_UM_INCLUDE_LINUX_WAIT_H__
#define __ARCH_UM_INCLUDE_LINUX_WAIT_H__

#ifndef __ASSEMBLER__

/* copied from wait.h */

#if defined __GNUC__ && !defined __cplusplus
#define __WAIT_INT(status) \
  (__extension__ (((union { __typeof(status) __in; int __i; }) \
                   { .__in = (status) }).__i))
#else
#define __WAIT_INT(status)	(*(__const int *) &(status))
#endif

/* copied from bits/waitflags.h  */

/* Bits in the third argument to `waitpid'.  */
#define	WNOHANG		1	/* Don't block waiting.  */
#define	WUNTRACED	2	/* Report status of stopped children.  */

/* Bits in the fourth argument to `waitid'.  */
#define WSTOPPED	2	/* Report stopped child (same as WUNTRACED). */
#define WEXITED		4	/* Report dead child.  */
#define WCONTINUED	8	/* Report continued child.  */
#define WNOWAIT		0x01000000	/* Don't reap, just poll status.  */

#define __WNOTHREAD     0x20000000	/* Don't wait on children of other threads
					   in this group */
#define __WALL		0x40000000	/* Wait for any child.  */
#define __WCLONE	0x80000000	/* Wait for cloned process.  */

/* wait status getter. copied from bits/waitstatus.h */

/* If WIFEXITED(STATUS), the low-order 8 bits of the status.  */
#define	__WEXITSTATUS(status)	(((status) & 0xff00) >> 8)

/* If WIFSIGNALED(STATUS), the terminating signal.  */
#define	__WTERMSIG(status)	((status) & 0x7f)

/* If WIFSTOPPED(STATUS), the signal that stopped the child.  */
#define	__WSTOPSIG(status)	__WEXITSTATUS(status)

/* Nonzero if STATUS indicates normal termination.  */
#define	__WIFEXITED(status)	(__WTERMSIG(status) == 0)

/* Nonzero if STATUS indicates termination by a signal.  */
#define __WIFSIGNALED(status) \
  (((signed char) (((status) & 0x7f) + 1) >> 1) > 0)

/* Nonzero if STATUS indicates the child is stopped.  */
#define	__WIFSTOPPED(status)	(((status) & 0xff) == 0x7f)

/* Nonzero if STATUS indicates the child continued after a stop.  We only
   define this if <bits/waitflags.h> provides the WCONTINUED flag bit.  */
#define __WIFCONTINUED(status)	((status) == __W_CONTINUED)

/* Nonzero if STATUS indicates the child dumped core.  */
#define	__WCOREDUMP(status)	((status) & __WCOREFLAG)

/* Macros for constructing status values.  */
#define	__W_EXITCODE(ret, sig)	((ret) << 8 | (sig))
#define	__W_STOPCODE(sig)	((sig) << 8 | 0x7f)
#define __W_CONTINUED		0xffff
#define	__WCOREFLAG		0x80

#define WEXITSTATUS(status)	__WEXITSTATUS(__WAIT_INT(status))
#define WTERMSIG(status)	__WTERMSIG(__WAIT_INT(status))
#define WSTOPSIG(status)	__WSTOPSIG(__WAIT_INT(status))
#define WIFEXITED(status)	__WIFEXITED(__WAIT_INT(status))
#define WIFSIGNALED(status)	__WIFSIGNALED(__WAIT_INT(status))
#define WIFSTOPPED(status)	__WIFSTOPPED(__WAIT_INT(status))
#define WIFCONTINUED(status)	__WIFCONTINUED(__WAIT_INT(status))

#endif /* __ASSEMBLER__ */

#endif /* !__ARCH_UM_INCLUDE_LINUX_WAIT_H__ */
