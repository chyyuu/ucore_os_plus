#include <vmm.h>
#include <string.h>
#include <mmu.h>
#include <stdio.h>
#include <kio.h>

bool
copy_from_user(struct mm_struct *mm, void *dst, const void *src, size_t len,
	       bool writable)
{
	if (!user_mem_check(mm, (uintptr_t) src, len, writable)) {
		return 0;
	}
	memcpy(dst, src, len);
	return 1;

}

bool copy_to_user(struct mm_struct * mm, void *dst, const void *src, size_t len)
{
	if (!user_mem_check(mm, (uintptr_t) dst, len, 1)) {
		return 0;
	}
	memcpy(dst, src, len);
	return 1;
}

bool copy_string(struct mm_struct * mm, char *dst, const char *src, size_t maxn)
{
	size_t alen, part =
	    ROUNDDOWN((uintptr_t) src + PGSIZE, PGSIZE) - (uintptr_t) src;
	while (1) {
		if (part > maxn) {
			part = maxn;
		}
		if (!user_mem_check(mm, (uintptr_t) src, part, 0)) {
			return 0;
		}
		if ((alen = strnlen(src, part)) < part) {
			/* kprintf ("copy range: 0x%x (len: 0x%x) to 0x%x\n", src, alen+1, dst); */
			memcpy(dst, src, alen + 1);
			/* kprintf ("copy range done\n"); */
			return 1;
		}
		if (part == maxn) {
			return 0;
		}
		memcpy(dst, src, part);
		dst += part, src += part, maxn -= part;
		part = PGSIZE;
	}
}
