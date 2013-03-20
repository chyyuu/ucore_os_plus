// ACPICA platform definitions for ucore_amd64

#pragma once

//#include <stdarg.h>
//#include <stdint.h>
#include <types.h>
#include <stdarg.h>

#include "platform/acgcc.h"

#if defined(__ia64__) || defined(__x86_64__)
#define ACPI_MACHINE_WIDTH          64
#else
#define ACPI_MACHINE_WIDTH          32
#error AMD64 only
#endif

#define COMPILER_DEPENDENT_INT64    int64_t
#define COMPILER_DEPENDENT_UINT64   uint64_t

// No matter what, don't use ACPICA's va_list!  It's subtly wrong, and
// ACPICA will silently swap it in if it thinks it doesn't have
// va_list.  This should be defined by stdarg.h, but be extra certain.
#ifndef va_arg
#error No va_arg
#endif

// Use ACPICA's built-in cache
#define ACPI_USE_LOCAL_CACHE

// Don't use a special mutex type
#define ACPI_MUTEX_TYPE ACPI_BINARY_SEMAPHORE

// Use ucore_amd64 spinlocks.  Unfortunately, these have to be heap-allocated
// because acpica expects a copyable handle.
struct spinlock;
#define ACPI_SPINLOCK struct spinlock *

// Use ucore_amd64 semaphores.
struct semaphore;
#define ACPI_SEMAPHORE struct semaphore *

