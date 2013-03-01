#include <vmm.h>
#include <types.h>
#include <host_syscall.h>
#include <proc.h>
#include <memlayout.h>
#include <pmm.h>
#include <mmu.h>
#include <string.h>

/**
 * Copy data from user space.
 * @param mm where the user data belongs to
 * @param dst destination where data should be copied to (kernel space)
 * @param src source address of the user data (user space)
 * @param len the length of data
 * @param writable whether the area should be writable
 * @return 1 if the operation succeeds, and 0 if not
 */
bool
copy_from_user(struct mm_struct *mm, void *dst, const void *src, size_t len,
	       bool writable)
{
	/* Make sure that the address is valid */
	if (!user_mem_check(mm, (uintptr_t) src, len, writable)) {
		return 0;
	}
	/* If no mm is specified, dst should be available in main process. */
	if (mm == NULL) {
		memcpy(dst, src, len);
		return 1;
	}

	/* Copy data page by page. */
	while (len > 0) {
		/* Touch the page so that its status will be right after copying. */
		if (host_getvalue(pls_read(current), (uintptr_t) src, NULL) < 0) {
			return 0;
		}

		/* Determine the size to copy */
		size_t avail = ROUNDUP(src, PGSIZE) - src;
		if (PGOFF(src) == 0)
			avail += PGSIZE;
		if (avail > len)
			avail = len;

		/* Calculate the offset in the tmp file where the data can be found. */
		pte_t *ptep = get_pte(mm->pgdir, (uintptr_t) src, 0);
		assert(ptep != NULL && (*ptep & PTE_P));
		uintptr_t addr = KERNBASE + PTE_ADDR(*ptep) + PGOFF(src);

		/* Now copy the data. */
		memcpy(dst, (void *)addr, avail);

		len -= avail;
		dst += avail;
		src += avail;
	}

	return 1;
}

/**
 * Write to user space.
 * @param mm the user memory space where we need to write to
 * @param dst the address where the data is copied to (user space)
 * @param src the address of the data to be written (kernel space)
 * @param len the length of the data
 * @return 1 if the operation succeeds, and 0 if not
 */
bool copy_to_user(struct mm_struct * mm, void *dst, const void *src, size_t len)
{
	/* Check whether the address specified is writable. */
	if (!user_mem_check(mm, (uintptr_t) dst, len, 1)) {
		return 0;
	}

	/* If no mm is specified, dst should be accessible in main process. */
	if (mm == NULL) {
		memcpy(dst, src, len);
		return 1;
	}

	/* Write data page by page. */
	while (len > 0) {
		/* Touch the page so that the status of the entry is right. */
		if (host_assign(pls_read(current), (uintptr_t) dst, 0) < 0)
			return 0;

		/* Determine the length which should be copied in the current page. */
		size_t avail = ROUNDUP(dst, PGSIZE) - dst;
		if (PGOFF(dst) == 0)
			avail += PGSIZE;
		if (avail > len)
			avail = len;

		/* Calculate the offset in the tmp file of the page. */
		pte_t *ptep = get_pte(mm->pgdir, (uintptr_t) dst, 0);
		assert(ptep != NULL && (*ptep & PTE_P));
		uintptr_t addr = KERNBASE + PTE_ADDR(*ptep) + PGOFF(dst);

		/* Carry out... */
		memcpy((void *)addr, src, avail);

		len -= avail;
		dst += avail;
		src += avail;
	}
	return 1;
}

/**
 * Copy a string ended with '\0' from user space.
 * @param mm the user space of the original string
 * @param dst destination of the string (in kernel space)
 * @param src pointer to the string (in user space)
 * @param maxn the max length of the string
 * @return 1 if succeeds and 0 otherwise
 */
bool copy_string(struct mm_struct * mm, char *dst, const char *src, size_t maxn)
{
	/* If no use space is specified, dst should be accessible in the main process. */
	if (mm == NULL) {
		int alen = strnlen((const char *)src, maxn);
		memcpy(dst, (void *)src, alen);
		*(dst + alen) = 0;
		return 1;
	}

	/* Copy the string page by page. */
	while (maxn > 0) {
		/* Determine the max length which can be copied in the current page. */
		size_t avail = ROUNDUP(src, PGSIZE) - src;
		if (PGOFF(src) == 0)
			avail += PGSIZE;
		if (avail > maxn)
			avail = maxn;

		/* Check whether the area is readable. */
		if (!user_mem_check(mm, (uintptr_t) src, avail, 0))
			goto fail;
		/* Touch the page to make the page entry right. */
		if (host_getvalue(pls_read(current), (uintptr_t) src, NULL) < 0)
			goto fail;

		/* Get the offset in the tmp file of the page where the string is */
		pte_t *ptep = get_pte(mm->pgdir, (uintptr_t) src, 0);
		assert(ptep != NULL && (*ptep & PTE_P));
		uintptr_t addr = KERNBASE + PTE_ADDR(*ptep) + PGOFF(src);

		/* Calculate the actual length to be copied and carry it out. */
		size_t alen = strnlen((const char *)addr, avail);
		memcpy(dst, (void *)addr, alen);

		maxn -= alen;
		dst += alen;
		src += alen;
		if (alen < avail)
			break;
	}

	/* the copied string should be ended with '\0'. */
	*dst = 0;
	return 1;

fail:
	*dst = 0;
	return 0;
}
