============
Driver Ucore
============

:Author: Mao Junjie <`eternal.n08@gmail.com <mailto:eternal.n08@gmail.com>`_>

Version
    $Revision: 1 $

This document discusses similarity and difference of drivers among ucore
implementations on different architectures.

Essential Devices And Their Interfaces
======================================

No matter what architecture ucore works on, there're several key devices
without which ucore won't work. They include

-  console output (serial port, LPT, CGA etc.)
-  input (serial port, keyboard etc.)
-  timer
-  disk (IDE, flash etc.)

Drivers are responsible for providing a set of pre-defined interfaces so
that the code above can operate on those devices regardless of what kind
of hardware ucore actually have. The interfaces are listed below.

Console
-------

A console is used to output messages as well as fetch characters from
users. As a result, there're three functions for initializing, reading
from and writing to a console.

void cons\_init(void)
    Initialize the console properly so that the device is ready for
    inputing and outputing.

void cons\_putc(int c)
    Writing a character to the device. *c* is the character in ascii
    which is to be printed.

int cons\_getc(void)
    Fetch a character. This should only be used when any interrupt
    indicates that there exist some characters to be read. The character
    is returned in ascii.

Timer
-----

A timer should, after initialzing, raise an interrupt periodically if
allowed.

void clock\_init(void)
    Initialize the timer so that once interrupt is enabled, timer
    interrupts should come.

size\_t ticks
    Record how many ticks have passed since ucore starts.

External Storage
----------------

File systems and swap partitions all reside on external storage. Because
of some historical reasons, the devices are all named IDE, though it can
actually be IDE disks, flash disks or even a part of memory.

Ucore may have multiple external storage, indexed by *ideno*.

void ide\_init(void)
    Initialize the block device.

bool ide\_device\_valid(unsigned short ideno)
    Check whether there exists any usable external storage *ideno*.

size\_t ide\_device\_size(unsigned short ideno)
    Return size of the storage represented by *ideno* in sector. The
    size of a sector can be different on different architecture but at
    present it is always 512 bytes.

int ide\_read\_secs(unsigned short ideno, uint32\_t secno, void \*dst,
size\_t nsecs)
    Read *nsecs* sectors start at *secno* from the device represented by
    *ideno* to *dst*.

int ide\_write\_secs(unsigned short ideno, uint32\_t secno, const void
\*src, size\_t nsecs)
    Write *nsecs* sectors start at *secno* to the device represented by
    *ideno* using *src*.

Implementation In Multi-arch Ucore
==================================

At present, every piece of driver code belongs to arch-dep part of
ucore, including the headers. There're still something requiring
consideration such as

-  Extract headers to arch-indep part.
-  Implement drivers as kernel modules.
