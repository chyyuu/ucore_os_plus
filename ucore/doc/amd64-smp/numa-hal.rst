===========
HAL on NUMA
===========

:Author: Mao Junjie <eternal.n08@gmail.com>
:Version: $Revision: 1 $

This document covers Linux HAL design for booting on NUMA platforms.

.. contents::

Platform Topology
=================

Cache Coherent NUMA
-------------------

On a ccNUMA platform,

  - all memory is visible to and accessible from any CPU on the platform and
  - Cache coherency is handled in hardware.

Linux is only primarily interested in ccNUMA platforms [1]_.

Linux CPU Topology Data Structures
----------------------------------

The CPU topology of a platform includes which of the processors are on the same core/package. The information is maintained in the following **per-CPU** CPU bitmaps.

**cpu_core_map**
  CPUs on the same package as this one.

**cpu_sibling_map**
  CPUs on the same core as this one.

NUMA-related topology is provided via the following interfaces.

**cpu_to_node(int cpu)**
  Get the node ID where *cpu* is. In x86 code, the information is saved in a per-cpu *x86_cpu_to_node_map* variable.

**cpumask_of_node(int node)**
  Get the set of CPUs in a specific node. In x86 code, this is saved in a global array of CPU bitmaps.

These bitmaps are initialized by each CPU itself (in *set_cpu_sibling_map*) It is based on the topology information collected from CPUID leaf 0xb (Extended Topology Enumeration Leaf) and APIC IDs (refer to *detect_extended_topology()* for details), which are collected after most low-level subsystems are initialized (the init entry in *start_kernel()* is *check_bugs* which invokes vendor-specific code).

Linux Memory Topology Data Structures
-------------------------------------

**struct numa_meminfo**
  The global struct *numa_meminfo* contains a finite set of *numa_memblk*s, each of which has a start address *start*, an end address *end* and the owner node ID *nid*.

**node_data**
  *node_data* is an array of *pglist_data*, each of which records the memory layout of one node.

Linux Bus Topology Data Structures
----------------------------------

The (PCI) bus topology is provided via the following interfaces.

**get_mp_bus_to_node(int busnum)**
  Get the node ID where the bus resides. In x86 code, the information is internally kept in the array *mp_bus_to_node*.

Other Topology Data Structures in Linux
---------------------------------------

**NUMA distance matrix**
  The distance matrix (*numa_distance*) is an x86 specific, line-major array that holds the distance information from ACPI SLIT table. The element in line *i* and column *j* is the distance from node *i* to node *j*. The information is accessed via *node_distance*.

xv6 CPU/Memory Topology Data Structures
---------------------------------------

**struct numa_node**
  Contains multiple CPUs (*struct cpu*) and multiple memory regions (*struct region*).

**struct cpu**
  Various states of each CPU. No topology information here except the numa node it is in.

Multi-core Operations
=====================

Linux CPU State Data Structure
------------------------------

In Linux, there are four bitmaps recording CPU states. The *nth* bit in the bitmap corresponds to processor id *n*.

**cpu_possible_mask**
  *n* is set iff *n* is polulatable. It is fixed at boot time. For example, if you boot with *cpu_possible_mask* being 0x00000003, the system can use no more than 2 logical CPUs even there are actually two sockets with four cores each. The only way to bring the other CPUs up is to reboot with a new *cpu_possible_mask* value.

**cpu_present_mask**
  *n* is set iff CPU *n* is populated. They can be either online or offline. This should be a subset of *cpu_possible_mask*.

**cpu_online_mask**
  *n* is set iff CPU *n* is online and ready for work. This should be a subset of *cpu_present_mask*.

**cpu_active_mask**
  The comment says it "has bit 'cpu' set iff cpu available to migration". Not sure what this means yet. :(

These states are designed with CPU hotplug into account. Without CPU hotplug feature, *cpu_possible_mask* = *cpu_present_mask* and *cpu_online_mask* = *cpu_active_mask*.

Linux SMP Operations
--------------------

In x86-specific code, smp-related interfaces are packed in a struct called *smp_ops*. The interfaces are list as follows, along with a brief introduction.

**void smp_prepare_boot_cpu(void)**
  Initialize the cpu state data structure and other miscellaneous things. This is called rather early in Linux common code (before *mm_init()*). The possible equivalence in xv6 is *initcpus()*.

**void smp_prepare_cpus(unsigned max_cpus)**
  Do some other miscellaneous things. On x86, this includes switching from PIC to APIC mode, setting up IOAPIC, etc.

**void smp_cpus_done(unsigned max_cpus)**
  Carry out any cleanup work after all CPUs have been brought up by *cpu_up()*.

**int cpu_up(unsigned cpu)**
  Bring up the CPU speficied by *cpu*. This interface is called for each non-boot CPU in common initialization code (i.e. *smp_init()*). The interface also takes charge of bringing up CPUs plugged in at runtime. The equivalence of *smp_init()* in xv6 is *bootothers()*.

**void stop_other_cpus(int wait)**
  Stop the other CPUs. This is usually used when rebooting the system. Wait all processors to be offline if *wait* is not zero.

**int cpu_disable(void)**
  Mark the current CPU as offline. It is related to CPU hotplug only.

**void cpu_die(unsigned int cpu)**
  Actually kill *cpu* to be plugged out. It should be considered as offline after this interface is called. It is related to CPU hotplug only.

**void play_dead(void)**
  Used in *cpu_idle()* when the current CPU is offline. The CPU should enter a power-saving mode. It is related to CPU hotplug only.

**void smp_send_reschedule(int cpu)**
  Make *cpu* reschedule immediately.

**void send_call_func_ipi(const struct cpumask \*mask)**
  Make all CPUs in *mask* calling a specific function. The function and its parameters are placed in a shared area.

**void send_call_func_single_ipi(int cpu)**
  Make *cpu* calling a specific function.

xv6 Multi-core Initialization Steps
-----------------------------------

**initnuma()**
  Construct NUMA information from ACPI tables.

**initpercpu()**
  Initialize per-cpu storage area. It is equivalant to *setup_per_cpu_areas()* in Linux.

**initcpus()**
  Add BP to NUMA information table.

**bootothers()**
  Bring up APs. It is equivalent to *smp_init()* in Linux.

uCore NUMA HAL Considerations
=============================

NUMA Topology Information
-------------------------

Possible solutions:

1. Generic data structures and access functions. Architecture-dependent code fills them.
2. Generic access functions only. Architecture-dependent code manages their own structures.

Multicore Interfaces
--------------------

The most important interface is *cpu_up()*. Use the helpers like *smp_prepare_cpus()* or *initcpus()* if necessary.

References
==========

.. [1] What is NUMA?, in Linux Kernel Documentation (Documentation/vm/numa)
.. [2] Linux Support for NUMA Hardware, in Proceedings of the Linux Symposium, 2003
.. [3] Linux Scalability Effort NUMA FAQ: http://lse.sourceforge.net/numa/faq
