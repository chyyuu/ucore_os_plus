===========================
 "Programming Guide" Ucore
===========================

:Author: Mao Junjie <eternal.n08@gmail.com>
:Version: $Revision: 1 $

This document discusses the rules to be followed when working multi-arch supported ucore.

.. contents::

Data Types In Common Code
-------------------------

Pay Attention To Integers
  There exist two sorts of integer types, e.g. the gcc builtin *int* and *int32_t*. The latter form is used when the variable is related to something outside the processor such as disks and network, as they should be able to cope with both 32-bit and 64-bit processors. On the other hand, those variables only used inside the processor should be declared as gcc builtin types. Especially, don't use *uint32_t* or *uint64_t* for addresses.

Endian In Common Code
---------------------

This is another problem of integers. Among the supported architectures of ucore, i386, x86_64 and um (on i386) all uses small-endian while or32 uses big-endian. When this issue is properly dealt with, there is no need for rebuilding the fs image when switching from i386 to or32. Ucore doesn't handle this at present however, and as a result, the images for i386 cannot be used for or32 (built with mksfs_be.c, not mksfs.c).

The standard interfaces (something like *htobe32*) for byte order operations can be found in the section 3 (Linux Programmer's Manual) of man pages. For example, to see the page in fedora, install the *man-pages* package via yum and execute *man htobe32*. Please kindly implement them if you need them. :)
