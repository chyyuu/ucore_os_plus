#ifndef __LIBS_TYPES_H__
#define __LIBS_TYPES_H__

#ifndef __NO_UCORE_TYPE__

#ifndef NULL
#define NULL ((void *)0)
#endif

#define CHAR_BIT        8

/* Represents true-or-false values */
typedef int bool;

/* Explicitly-sized versions of integer types */
typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

#if defined(__LP64__) || defined(__64BIT__) || defined(_LP64)
#ifndef __UCORE_64__
#define __UCORE_64__
#endif
#endif

/* *
 * We use pointer types to represent addresses,
 * uintptr_t to represent the numerical values of addresses.
 *
 * */

#ifdef __UCORE_64__

/* Machine word type. */
typedef uint64_t machine_word_t;

/* Pointers and addresses are 64 bits long in 64-bit platform. */
typedef int64_t intptr_t;
typedef uint64_t uintptr_t;

#else /* not __UCORE_64__ (only used for 32-bit libs) */

/* Machine word type. */
typedef uint32_t machine_word_t;

/* Pointers and addresses are 32 bits long in 32-bit platform. */
typedef int32_t intptr_t;
typedef uint32_t uintptr_t;

#endif /* !__UCORE_64__ */

/* size_t is used for memory object sizes. */
typedef uintptr_t size_t;

/* off_t is used for file offsets and lengths */
typedef intptr_t off_t;

/* ppn_t used for page numbers */
typedef size_t ppn_t;

/* *
 * Rounding operations (efficient when n is a power of 2)
 * Round down to the nearest multiple of n
 * */
#define ROUNDDOWN(a, n) ({                                          \
            size_t __a = (size_t)(a);                               \
            (typeof(a))(__a - __a % (n));                           \
        })

/* Round up to the nearest multiple of n */
#define ROUNDUP(a, n) ({                                            \
            size_t __n = (size_t)(n);                               \
            (typeof(a))(ROUNDDOWN((size_t)(a) + __n - 1, __n));     \
        })

/* Round up the result of dividing of n */
#define ROUNDUP_DIV(a, n) ({                                        \
            uint32_t __n = (uint32_t)(n);                           \
            (typeof(a))(((a) + __n - 1) / __n);                     \
        })

/* Return the offset of 'member' relative to the beginning of a struct type */
#define offsetof(type, member)                                      \
    ((size_t)(&((type *)0)->member))

/* *
 * to_struct - get the struct from a ptr
 * @ptr:    a struct pointer of member
 * @type:   the type of the struct this is embedded in
 * @member: the name of the member within the struct
 * */
#define to_struct(ptr, type, member)                               \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#ifndef __mode_t_defined
typedef uint32_t mode_t;
#define __mode_t_defined
#endif

typedef uint32_t dev_t;

#ifndef __user
#define __user
#endif

#define __XCONCAT2(a, b) a ## b
#define __XCONCAT(a, b) __XCONCAT2(a, b)

#endif /* __NO_UCORE_TYPE__ */

/* sem_t is used for semaphore id */
typedef uintptr_t sem_t;

#endif /* !__LIBS_TYPES_H__ */
