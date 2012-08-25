========
MM Ucore
========

:Author: Mao Junjie <eternal.n08@gmail.com>
:Version: $Revision: 1 $

This document describes memory management mechanism from the view of multi-arch.

.. contents::

Dependencies
============

Ucore's memory management relies on varies features, which are usually referred to as *MMU*, of the lower architecture. Those features include:

* Physical memory is divided into various block of the same size called *page*,
* Software-manageable map from virtual addresses to physical ones,
* Software-manageable page permission,
* Interrupts raised when an illegal access to a page happens.

As long as these requirements are met, it is not important how they are satisfied. For example, while on *i386*, *x86_64* and *or32* there is a page table on which both software and hardware operates, host's VMM is used on *um* to fulfill the tasks mentioned above. The page table is also maintained though it has nothing to do with address mapping.

.. Note:: Ucore has also been partially ported to a simple arm implementation on FPGA which has no address mapping mechanism. The rules above may need more consideration. For more details, refer to [#]_.

Page Table
==========
The page table defined in ucore is regarded as an interface to the above arch-indep code such as virtual memory management. Some key points to it include:

* Multi-level. The table can has at leat one level and at most four levels.
* Entry per page. Each page in virtual address space corresponds to one and only one entry.
* Accessors instead of bit operation. See below.

Page Table Entry Accessors
--------------------------

A page table entry usually contains not only a physical page frame number but also some permission control bits. Ucore's virtual memory management relies heavily on those bits while on different architecture, definitions of those bits may be totally different. To make things worse, a one-bit operation on one arch such as mark a page as readable for users may requires modifying several bits on another one.

A set of operations is then introduced so that ucore's VMM can work without any knowledge of bit definitions of local page entries.

.. Note:: Only basic operations are listed here. As ucore is originally designed for i386, there's no control on whether a page is executable and supervisor-readable.

.. Note:: The arch-dep part of ucore may still using bit operations on page table entries. This won't prevent ucore from running normally but should be fixed.

.. Note:: All return values of the functions listed below (if it has one) are considered as typical boolean type in C, i.e. *0* means *false*, while any non-zero values means *true* (DO NOT assume that it is 1).

Basic Properties
~~~~~~~~~~~~~~~~

A page table entry may contain an available physical page address, a number of a page swapped out, or totally nothing.

int ptep_invalid(pte_t \*ptep)
  Whether the entry contains nothing. Usually an empty entry means an entry filled with 0.

int ptep_present(pte_t \*ptep)
  Whether the physical page is available at once. An valid but not present entry should be used for swapping.

void ptep_map(pte_t \*ptep, uintptr_t pa)
  Fill in the entry using given physical page. *pa* should be page-aligned. The page will be readable and executable at once for supervisor while no permission is given to user.

void ptep_unmap(pte_t \*ptep)
  Clear an entry. The entry will become invalid after unmap.

Permission Getters
~~~~~~~~~~~~~~~~~~

Set permissions for a certain page. Possible permission levels include:

* Supervisor readable
* Supervisor writable / User readable
* User writable

Note that a latter level implies all permissions of the formers.

int ptep_s_write(pte_t \*ptep)
  Return whether the page is writable to supervisor. What it returns for swap entries is meaningless and undefined.

int ptep_u_read(pte_t \*ptep)
  Return whether the page is readable to user. What it returns for swap entries is meaningless and undefined.

int ptep_u_write(pte_t \*ptep)
  Return whether the page is writable to user. What it returns for swap entries is meaningless and undefined.

Permission Setters
~~~~~~~~~~~~~~~~~~

The set of functions below set or unset a certain permission.

void ptep_set_s_write(pte_t \*ptep)
  Enable supervisor to write the page. What it does for swap entries or invalid entries is undefined.

void ptep_set_u_read(pte_t \*ptep)
  Enable user to read the page. If the page is writable to supervisor, user writing is also allowed at the same time. What it does for swap entries or invalid entries is undefined.

void ptep_set_u_write(pte_t \*ptep)
  Enable user to write the page, implying that supervisor is also allowed to write it. What it does for swap entries or invalid entries is undefined.

void ptep_unset_s_write(pte_t \*ptep)
  Disable supervisor from writing the page. User is also prohibited at the same time. What it does for swap entries or invalid entries is undefined.

void ptep_unset_u_read(pte_t \*ptep)
  Disable user from reading the page, implying that user cannot write to it any longer. What it does for swap entries or invalid entries is undefined.

void ptep_unset_u_write(pte_t \*ptep)
  Disable user from writing the page, making the page not writable to supervisor at the same time. User can still read the page if it is allowed before the function is called. What it does for swap entries or invalid entries is undefined.

Page Access History Records
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Page access history in ucore includes whether a page has been accessed, regardless of reading or writing, and whether a page has been written (dirty). These two properties should be independent, i.e. a page may be recorded as dirty and not accessed at the same time.

int ptep_accessed(pte_t \*ptep)
  Return whether the page has been accessed since its creation or when it is set unaccessed. What it returns for swap or invalid entries is undefined.

int ptep_dirty(pte_t \*ptep)
  Return whether the page has been written since its creation or when it is set clean. What it returns for swap or invalid entries is undefined.

void ptep_set_accessed(pte_t \*ptep)
  Set the page as having been accessed. This is used only on architectures that don't support setting such properties automatically such as *um* and *or32*. What it does for swap or invalid entries is undefined.

void ptep_unset_accessed(pte_t \*ptep)
  Set the page as not having been accessed. What it does for swap or invalid entries is undefined.

void ptep_set_dirty(pte_t \*ptep)
  Set the page as dirty. This is used only on architectures that don't support setting such properties automatically such as *um* and *or32*. What it does for swap or invalid entries is undefined.

void ptep_unset_dirty(pte_t \*ptep)
  Set the page as clean. What it does for swap or invalid entries is undefined.

Permission Set
~~~~~~~~~~~~~~

When inserting a page into a page table in ucore, it is common to pass page settings through several functions, each of which setting or clearing some of its properties. In order to distinguish those settings from real page table entries, another type *pte_perm_t* is defined. It is possible to use functions listed above for checking, setting or clearing its properties and finally apply it to a real entry.

pte_perm_t ptep_get_perm(pte_t \*ptep, pte_perm_t perm)
  Get the permissions of a page. The permission is set only both *ptep* and *perm* have it, so *perm* can be regarded as a mask.

void ptep_set_perm(pte_t \*ptep, pte_perm_t perm)
  Set permissions in *ptep* if *perm* has it. No permissions is unset during the operation.

void ptep_unset_perm(pte_t \*ptep, pte_perm_t perm)
  Unset permissions in *petp* if *perm* doesn't has it. No permissions is set during the operation.

Misc
~~~~

void ptep_copy(pte_t \*to, pte_t \*from)
  Make a copy of the table entry. It simply do the assignment no matter what architecture ucore is built for at present. It is here so that the arch-indep code never dereferencing any pte_t\*.

Virtual Memory Management (VMM)
===============================

Most functional part of VMM is arch-indep. The only functions that may differ is copying from/to userspace in kernel. The key reason here is that on architectures such as *i386*, the userspace is fully available when the kernel is invoked via syscall, while on *um*, this is not the case. Thus, copy data from/to userspace requires more sophisticated mechanism (see *um*'s implementation for details), which is totally different from the others. There're three functions falling into this category:

* **copy_from_user**
* **copy_to_user**
* **copy_string**

Swap, Shared Memory and Others
==============================
All functional parts of those subsystems are arch-indep. The arch-dep part only has tests.

References
==========

.. [#] https://github.com/thinxer/ucore-multi
