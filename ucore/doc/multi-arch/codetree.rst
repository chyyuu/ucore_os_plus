===============
Code-tree Ucore
===============

:Author: Mao Junjie <eternal.n08@gmail.com>
:Author: Chen Yu (chyyuu@gmail.com)
:Version: $Revision: 1.1 $

This document describes the code tree from the view of multi-arch.

.. contents::

General
=======

Ucore is currently divided into various modules, each of which will be built seperately and then merged together by the instructions specified in the architecture-specific makefile. Most modules are divided into two parts: the architecture-dependent (will be abbreviated to arch-dep below) part and the architecture-independent (will be abbreviated to arch-indep below) part. The former is shared among all implementations, while the latter is only used for specific architecture. Attached to a module's top directory, the arch-indep part are located in all directories except arch, and the arch-dep part are located in arch/$ARCH where ARCH is the architecture's name (this will be used through this document).

The arch-dep part of each module usually at least contains a makefile and a link script.

The relative paths used below are all based on the assumption that it is attached to the module's top directory.

Bootloader
==========

The arch-dep part only includes elf headers and common type definitions. All executable code are located in arch/$ARCH.

For *i386*, *amd64* and *or32*, this is a real bootloader which will be located on the first block of external storage. The only function of it is to load the kernel in elf format from external storage to a pre-defined location in memory.

For *um*, the so-called bootloader is actually a user interface. It parses arguments given, initialize the environment, load the kernel and finally start running it.

Kernel
======

This section discusses the subsystems in the arch-dep part of the kernel (the module *kern-ucore*).

Debug
-----

Located at arch/$ARCH/debug.

This subsystem should at least provide **__warn** and **__panic** which is used by **panic** and **assert**. Extra debug support may be included such as the kernel debug monitor in *i386* and *amd64*.

Drivers
-------

Located at arch/$ARCH/driver.

This subsystem provides basic drivers for essential devices on which ucore heavily depends. Refer to the manual *Drivers* for details.

Interrupt configuration is usually included in this subsystem. Files named intr.c or picirq.c are related.

Initialization
--------------

Located at arch/$ARCH/init for *i386*, *or32* and *um*, and at arch/$ARCH/glue_ucore for *amd64*.

This subsystem usually provides an entry.c/init.c/main.c. It includes the C side kernel entry which initializes ucore's other subsystems and start running user processes.

Something else may be included. For example, in *or32* it has interrupt vectors one of which is the very start of the whole kernel.

Memory Management
-----------------

Located at arch/$ARCH/mm.

Usually has a pmm.c which at least includes mmu initialization and TLB invalidation, and vmm.c which implements **copy_from_user**, **copy_to_user** and **copy_string**. Refer to the manual **mm** for details.

The file swap.c usually appears too. This file has actually nothing functional but only the built-in test for swap subsystem.

Synchronization
---------------

Located at arch/$ARCH/sync.

Only provide **__intr_save** and **__intr_restore**. This is arch-dep because in **__intr_save**, different methods are used to check whether interrupts are disabled at present. Maybe it is possible to provide such a checker so that these code's movement to arch-indep part is perfectly justified.

System Call
-----------

Located at arch/$ARCH/syscall.

The only piece of code that is truly arch-dep is **syscall** because trapframes may be different on different architectures. Because other functions and global variables are static, everything is thus moved here. It should possible to expose only the registration table.

Process
-------

Located at arch/$ARCH/process.

This part provides at least **switch_to** in switch.S, **kernel_thread_entry** in procentry.S and the interfaces defined for process subsystem in proc.c. Refer to the manual *process* for details.


User Library (ulib)
===================

Ulib is a library which is at present statically linked into all user programs. The arch-dep parts of this library include:

* Program entry, which is usually prepare for *argc* and *argv* and call umain. (arch/$ARCH/initcode.S)
* Entry of cloned processes. (arch/$ARCH/clone.S)
* Atomic operations. This is used for userspace locks. (arch/$ARCH/atomic.h)
* Syscall invocation. (arch/$ARCH/syscall.c)
* The **do_div** macro or function for carrying out division. (arch/$ARCH/arch.h)
* Anything needed to provide the functions above.

Userspace Test Cases
====================

User programs should be totally unaware of what platform it is running on if a library is properly designed and provided. It is so for most programs except *badsegment.c* and *softint.c* which use inline assembly to test ucore's error handling. Thus, they are now put into archive and won't be compiled into the fs image by default.

