// Per-CPU variables with static duration

// We isolate percpu variables in a special section called .percpu,
// which kernel.ld places just after the bss.  The .percpu section in
// the kernel image itself becomes the per-CPU storage for the boot
// CPU.  For other CPUs, we allocate a separate region in that CPU's
// NUMA node of the same size as .percpu.  Then, to find a CPU's
// instance of a per-CPU variable, we simply offset that variable's
// address from the .percpu section into the other CPU's region.  This
// approach lets the linker do most of the heavy lifting for us, packs
// per-CPU data nice (unlike creating arrays for each variable), and
// makes it easy to associate per-CPU variables with the appropriate
// NUMA nodes.


#ifndef __PERCPU_H
#define __PERCPU_H
#include <arch.h>

// This is like DEFINE_PERCPU, but doesn't call the class's
// constructor.  This is for special cases like cpus that must be
// initialized remotely.
#define DEFINE_PERCPU_NOINIT(type, name)                           \
  type __percpu_##name __attribute__((PERCPU_SECTION)); \

#define DECLARE_PERCPU(type, name) \
  extern type __percpu_##name;                \

// The base of each CPU's per-CPU region.  This is used to find other
// CPU's regions.  This pointer is also stored in struct cpu; that
// copy is used to quickly find our own CPU's region.
// defined in mp.c
extern void *percpu_offsets[NCPU];

// The base of core 0's percpu variable block.  Provided by the
// linker.
extern char __percpu_start[];
extern char __percpu_end[];

#define __my_cpu_offset (percpu_offsets[myid()])

/* TODO: we should disable preempt if the kernel is preemptable,
 * because thread migration will render a percpu var invalid
 */
#define get_cpu_var(var) (*(typeof(&__percpu_##var))((char*)(&__percpu_##var) - __percpu_start + __my_cpu_offset))
#define get_cpu_ptr(var) (&get_cpu_var(var))

/* helper to get other cpu's percpu var */
#define per_cpu(var, id) (*(typeof(&__percpu_##var))((char*)(&__percpu_##var) - __percpu_start + percpu_offsets[id]))
#define per_cpu_ptr(var, id) (&per_cpu(var,id))


#endif

