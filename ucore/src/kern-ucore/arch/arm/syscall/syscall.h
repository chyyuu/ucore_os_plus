#ifndef __KERN_SYSCALL_SYSCALL_H__
#define __KERN_SYSCALL_SYSCALL_H__

#include <trap.h>
void syscall();

#define __sys2(x) #x
#define __sys1(x) __sys2(x)

#ifndef __syscall
#define __syscall(name) "swi\t" __sys1(SYS_##name) "\n\t"
#endif

#define __syscall_return(type, res)                                      \
do {                                                                              \
if ((unsigned long)(res) >= (unsigned long)(-125)) {           \
           res = -1;                                                  \
}                                                                         \
return (type) (res);                                                 \
} while (0)

#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3)           \
type name(type1 arg1,type2 arg2,type3 arg3) {                                  \
  long __res;                                                                     \
  __asm__ __volatile__ (                                                        \
  "mov\tr0,%1\n\t"                                                                  \
  "mov\tr1,%2\n\t"                                                                  \
  "mov\tr2,%3\n\t"                                                                  \
  __syscall(name)                                                            \
  "mov\t%0,r0"                                                                         \
        : "=r" (__res)                                                             \
        : "r" ((long)(arg1)),"r" ((long)(arg2)),"r" ((long)(arg3))     \
        : "r0","r1","r2","lr");                                                       \
  __syscall_return(type,__res);                                                     \
}

#define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4)           \
type name(type1 arg1,type2 arg2,type3 arg3, type4 arg4) {                                  \
  long __res;                                                                     \
  __asm__ __volatile__ (                                                        \
  "mov\tr0,%1\n\t"                                                                  \
  "mov\tr1,%2\n\t"                                                                  \
  "mov\tr2,%3\n\t"                                                                  \
  "mov\tr3,%4\n\t"                                                                  \
  __syscall(name)                                                            \
  "mov\t%0,r0"                                                                         \
        : "=r" (__res)                                                             \
        : "r" ((long)(arg1)),"r" ((long)(arg2)),"r" ((long)(arg3)),"r"((long)(arg4))     \
        : "r0","r1","r2","r3","lr");                                                       \
  __syscall_return(type,__res);                                                     \
}

//_syscall3(int, exec, const char*, name, int, argc, const char**, argv);

#endif /* !__KERN_SYSCALL_SYSCALL_H__ */
