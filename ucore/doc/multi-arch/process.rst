=============
Process Ucore
=============

:Author: Mao Junjie <eternal.n08@gmail.com>
:Version: $Revision: 1 $

This document describes the current interfaces defined in the process management subsystem.

.. contents::

Introduction
============

The arch-dep code in the process management subsystem usually include

* Definition of *context* and *arch_proc_struct* structures (arch/$ARCH/process/arch_proc.h)
* Context switch (arch/$ARCH/process/switch.S)
* Kernel thread's entry (arch/$ARCH/process/procentry.S)
* Some other interfaces (arch/$ARCH/process/proc.c, see below)

Basic Interfaces
================

struct proc_struct \*alloc_proc(void)
  Create and return a PCB. All fields should be initialized properly. The initialization of arch-indep fields in PCB can actually be moved to arch-indep code but it is not so yet.

void switch_to(struct context \*from, struct context \*to)
  Switch from context *from* to context *to*. What a 'context' contains is defined by arch-dep code. This function is usually a global symbol in an assembly source file such as arch/$ARCH/process/switch.S.

int kernel_thread(int (\*fn)(void \*), void \*arg, uint32_t clone_flags)
  Start a kernel thread, running from the function *fn* with arguments *arg*. The function should create a new process, register it in the process list and returns. The thread created will be executed when the scheduler let it to.

int copy_thread(uint32_t clone_flags, struct proc_struct \*proc, uintptr_t esp, struct trapframe \*tf)
  This is called at the end of forking a new process, setting some key fields in the trapframe and context. It will simply returns and the newly created process will be executed once the scheduler let it to.

int init_new_context(struct proc_struct \*proc, struct elfhdr \*elf, int argc, char\*\* kargv)
  This function is called when 'do_execve' is executed. It initialize the context of *proc* using the giving information.

int kernel_execve(const char \*name, const char \*\*argv)
  Call 'do_execve' in kernel. This is used when kernel wants to create a new thread which starts from a given ELF file.

void cpu_idle(void)
  This function is called after the kernel has initialized everything properly. It should never returns, looping until scheduling is required. For its implementation is a bit different in ucore-mp64, it is moved to arch-dep code for simplicity. After a careful look into the differences, it is advised that this function be moved back to the arch-indep part.

Special Hooks For *UM*
======================

These hooks described below are currently only used by *um* because of its own process-emulating mechanism. In other architecture implementations, they are simply empty functions or returns 0 if a return value is required.

void de_thread_arch_hook(struct proc_struct \*proc)
  This function is called when detaching a thread from a process which happens when a thread exits or called 'do_execve'. It is needed because in *um*, there're more resources to be freed sometime, such as the corresponding host process if it contains no threads any longer.

int do_execve_arch_hook(int argc, char \*\* argv)
  This function is called at the end of 'do_execve'. It is needed because instead of returning from the trap, *um* has another way to start a process (refer to its implementation for details). This function never returns if the process is successfully started. When it does return, it means that something bad happens and the kernel is responsible for freeing what we have asked for.
