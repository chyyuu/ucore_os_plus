===========
Build Ucore
===========

:Author: Mao Junjie <eternal.n08@gmail.com>
:Version: $Revision: 1 $

This document discusses how to build ucore for different architectures and makefiles used.

.. contents::

Building Ucore
==============

To build ucore for x86_64 and simulate the kernel, simply type

    make ARCH=x86_64

    make qemu

All dependencies, objects, executables and images will be created under obj.

Makefile Layout
===============
Makefiles used at present are derived from ucore-mp64 project [#]_. They include:

* top level makefiles

  - one main makefile (Makefile)
  - compilation-related variables (mk/comp\*.mk)
  - architecture-related configs (mk/config-\*.mk)
  - architecture-based building targets (mk/arch-\*.mk)

* per-module makefiles

  - one main makefile (src/$MODULE/Makefile)
  - architecture-dependent variables and targets (src/$MODULE/arch/$ARCH/\*.mk)

Variables
=========

ARCH
  What architecture **ucore** is built for. Possible choices include **i386**, **x86_64**, **or32** and **um**.

HOST_CC_PREFIX
  Prefix of compiler, assembler etc. for building host tools. Default to empty in all architectures.

TARGET_CC_PREFIX
  Prefix of compiler, assembler etc. for buildiing ucore. Default to empty in i386 and um, "x86_64-elf-" in x86_64 and "or32-elf-" in or32.

Targets
=======

mod-$MODULE
  For each module needed, there exists a target named mod-$MODULE which simply print a few information and invoke the module's makefile. 

clean
  Cleans everything under directory obj.

qemu
  Simulate the kernel using qemu. Available when **ARCH** is i386 or x86_64.

run
  Execute the kernel directly or using a simulator other than qemu. Available when **ARCH** is um or or32.

others
  Extra targets can also be defined in order to create images or do anything else.

References
==========

.. [#] https://github.com/xinhaoyuan/ucore-mp64
