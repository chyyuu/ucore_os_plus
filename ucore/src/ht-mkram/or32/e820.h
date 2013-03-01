#ifndef E820_H
#define E820_H

typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef uint32_t size_t;

#define E820MAX             10
#define E820_ARM            1	// address range memory
#define E820_ARR            2	// address range reserved

struct e820map {
	int nr_map;
	struct {
		uint64_t addr;
		uint64_t size;
		uint32_t type;
	} map[E820MAX];
};

#define PGSIZE 0x2000

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

#endif /* !E820_H */
