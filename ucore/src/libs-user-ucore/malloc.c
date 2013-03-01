#include <types.h>
#include <ulib.h>
#include <syscall.h>
#include <malloc.h>
#include <lock.h>
#include <unistd.h>

// A thread-awared memory allocator based on  the 
// memory allocatorby Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

void lock_fork(void);
void unlock_fork(void);

union header {
	struct {
		union header *ptr;
		size_t size;
		bool type;	// 0: normal, sys_brk; 1: shared memory, shmem
	} s;
	uint32_t align[16];
};

typedef union header header_t;

static header_t base;
static header_t *freep = NULL;

static lock_t mem_lock = INIT_LOCK;

static void free_locked(void *ap);

static inline void lock_malloc(void)
{
	lock_fork();
	lock(&mem_lock);
}

static inline void unlock_malloc(void)
{
	unlock(&mem_lock);
	unlock_fork();
}

static bool morecore_brk_locked(size_t nu)
{
	static uintptr_t brk = 0;
	if (brk == 0) {
		if (sys_brk(&brk) != 0 || brk == 0) {
			return 0;
		}
	}
	uintptr_t newbrk = brk + nu * sizeof(header_t);
	if (sys_brk(&newbrk) != 0 || newbrk <= brk) {
		return 0;
	}
	header_t *p = (void *)brk;
	p->s.size = (newbrk - brk) / sizeof(header_t);
	p->s.type = 0;
	free_locked((void *)(p + 1));
	brk = newbrk;
	return 1;
}

static bool morecore_shmem_locked(size_t nu)
{
	size_t size = ((nu * sizeof(header_t) + 0xfff) & (~0xfff));
	uintptr_t mem = 0;
	if (sys_shmem(&mem, size, MMAP_WRITE) != 0 || mem == 0) {
		return 0;
	}
	header_t *p = (void *)mem;
	p->s.size = size / sizeof(header_t);
	p->s.type = 1;
	free_locked((void *)(p + 1));
	return 1;
}

static void *malloc_locked(size_t size, bool type)
{
	static_assert(sizeof(header_t) == 0x40);
	header_t *p, *prevp;
	size_t nunits;

	// make sure that type is 0 or 1
	if (type) {
		type = 1;
	}

	nunits = (size + sizeof(header_t) - 1) / sizeof(header_t) + 1;
	if ((prevp = freep) == NULL) {
		base.s.ptr = freep = prevp = &base;
		base.s.size = 0;
	}

	for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr) {
		if (p->s.type == type && p->s.size >= nunits) {
			if (p->s.size == nunits) {
				prevp->s.ptr = p->s.ptr;
			} else {
				header_t *np = prevp->s.ptr = (p + nunits);
				np->s.ptr = p->s.ptr;
				np->s.size = p->s.size - nunits;
				np->s.type = type;
				p->s.size = nunits;
			}
			freep = prevp;
			return (void *)(p + 1);
		}
		if (p == freep) {
			bool(*morecore_locked) (size_t nu);
			morecore_locked =
			    (!type) ? morecore_brk_locked :
			    morecore_shmem_locked;
			if (!morecore_locked(nunits)) {
				return NULL;
			}
		}
	}
}

static void free_locked(void *ap)
{
	header_t *bp = ((header_t *) ap) - 1, *p;

	for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr) {
		if (p >= p->s.ptr && (bp > p || bp < p->s.ptr)) {
			break;
		}
	}

	if (bp->s.type == p->s.ptr->s.type && bp + bp->s.size == p->s.ptr) {
		bp->s.size += p->s.ptr->s.size;
		bp->s.ptr = p->s.ptr->s.ptr;
	} else {
		bp->s.ptr = p->s.ptr;
	}

	if (p->s.type == bp->s.type && p + p->s.size == bp) {
		p->s.size += bp->s.size;
		p->s.ptr = bp->s.ptr;
	} else {
		p->s.ptr = bp;
	}

	freep = p;
}

void *malloc(size_t size)
{
	void *ret;
	lock_malloc();
	ret = malloc_locked(size, 0);
	unlock_malloc();
	return ret;
}

void *shmem_malloc(size_t size)
{
	void *ret;
	lock_malloc();
	ret = malloc_locked(size, 1);
	unlock_malloc();
	return ret;
}

void free(void *ap)
{
	lock_malloc();
	free_locked(ap);
	unlock_malloc();
}
