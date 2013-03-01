#include <types.h>
#include <shmem.h>
#include <slab.h>
#include <sync.h>
#include <pmm.h>
#include <string.h>
#include <swap.h>
#include <error.h>
#include <sem.h>

struct shmem_struct *shmem_create(size_t len)
{
	struct shmem_struct *shmem = kmalloc(sizeof(struct shmem_struct));
	if (shmem != NULL) {
		list_init(&(shmem->shmn_list));
		shmem->shmn_cache = NULL;
		shmem->len = len;
		set_shmem_ref(shmem, 0);
		sem_init(&(shmem->shmem_sem), 1);
	}
	return shmem;
}

static shmn_t *shmn_create(uintptr_t start)
{
	assert(start % PGSIZE * SHMN_NENTRY == 0);
	shmn_t *shmn = kmalloc(sizeof(shmn_t));
	if (shmn != NULL) {
		struct Page *page;
		if ((page = alloc_page()) != NULL) {
			shmn->entry = (pte_t *) page2kva(page);
			shmn->start = start;
			shmn->end = start + PGSIZE * SHMN_NENTRY;
			memset(shmn->entry, 0, PGSIZE);
		} else {
			kfree(shmn);
			shmn = NULL;
		}
	}
	return shmn;
}

static inline void shmem_remove_entry_pte(pte_t * ptep)
{
	//TODO
	//assert(0);
	assert(ptep != NULL);
	if (ptep_present(ptep)) {
		struct Page *page = pte2page(*ptep);
#ifdef UCONFIG_SWAP
		if (!PageSwap(page)) {
			if (page_ref_dec(page) == 0) {
				free_page(page);
			}
		} else {
			if (ptep_dirty(ptep)) {
				SetPageDirty(page);
			}
			page_ref_dec(page);
		}
#else
		if (page_ref_dec(page) == 0) {
			free_page(page);
		}
#endif /* UCONFIG_SWAP */
		ptep_unmap(ptep);
	} else if (!ptep_invalid(ptep)) {
#ifdef UCONFIG_SWAP
		swap_remove_entry(*ptep);
		ptep_unmap(ptep);
#else
		assert(0);
#endif
	}
}

static void shmn_destroy(shmn_t * shmn)
{
	int i;
	for (i = 0; i < SHMN_NENTRY; i++) {
		shmem_remove_entry_pte(shmn->entry + i);
	}
	free_page(kva2page(shmn->entry));
	kfree(shmn);
}

static shmn_t *find_shmn(struct shmem_struct *shmem, uintptr_t addr)
{
	shmn_t *shmn = shmem->shmn_cache;
	if (!(shmn != NULL && shmn->start <= addr && addr < shmn->end)) {
		shmn = NULL;
		list_entry_t *list = &(shmem->shmn_list), *le = list;
		while ((le = list_next(le)) != list) {
			shmn_t *tmp = le2shmn(le, list_link);
			if (tmp->start <= addr && addr < tmp->end) {
				shmn = tmp;
				break;
			}
		}
	}
	if (shmn != NULL) {
		shmem->shmn_cache = shmn;
	}
	return shmn;
}

static inline void check_shmn_overlap(shmn_t * prev, shmn_t * next)
{
	assert(prev->start < prev->end);
	assert(prev->end <= next->start);
	assert(next->start < next->end);
}

static void insert_shmn(struct shmem_struct *shmem, shmn_t * shmn)
{
	list_entry_t *list = &(shmem->shmn_list), *le = list;
	list_entry_t *le_prev = list, *le_next;
	while ((le = list_next(le)) != list) {
		shmn_t *shmn_prev = le2shmn(le, list_link);
		if (shmn_prev->start > shmn->start) {
			break;
		}
		le_prev = le;
	}

	le_next = list_next(le_prev);

	/* check overlap */
	if (le_prev != list) {
		check_shmn_overlap(le2shmn(le_prev, list_link), shmn);
	}
	if (le_next != list) {
		check_shmn_overlap(shmn, le2shmn(le_next, list_link));
	}

	list_add_after(le_prev, &(shmn->list_link));
}

void shmem_destroy(struct shmem_struct *shmem)
{
	list_entry_t *list = &(shmem->shmn_list), *le;
	while ((le = list_next(list)) != list) {
		list_del(le);
		shmn_destroy(le2shmn(le, list_link));
	}
	kfree(shmem);
}

pte_t *shmem_get_entry(struct shmem_struct *shmem, uintptr_t addr, bool create)
{
	assert(addr < shmem->len);
	addr = ROUNDDOWN(addr, PGSIZE);
	shmn_t *shmn = find_shmn(shmem, addr);

	assert(shmn == NULL || (shmn->start <= addr && addr < shmn->end));
	if (shmn == NULL) {
		uintptr_t start = ROUNDDOWN(addr, PGSIZE * SHMN_NENTRY);
		if (!create || (shmn = shmn_create(start)) == NULL) {
			return NULL;
		}
		insert_shmn(shmem, shmn);
	}
	int index = (addr - shmn->start) / PGSIZE;
	if (shmn->entry[index] == 0) {
		if (create) {
			struct Page *page = alloc_page();
			if (page != NULL) {
				ptep_map(&(shmn->entry[index]), page2pa(page));
				page_ref_inc(page);
			}
		}
	}
	return shmn->entry + index;
}

int shmem_insert_entry(struct shmem_struct *shmem, uintptr_t addr, pte_t entry)
{
	pte_t *ptep = shmem_get_entry(shmem, addr, 1);
	if (ptep == NULL) {
		return -E_NO_MEM;
	}
	if (!ptep_invalid(ptep)) {
		shmem_remove_entry_pte(ptep);
	}
	if (ptep_present(&entry)) {
		page_ref_inc(pte2page(entry));
	} else if (!ptep_invalid(&entry)) {
#ifdef UCONFIG_SWAP
		swap_duplicate(entry);
#else
		assert(0);
#endif
	}
	ptep_copy(ptep, &entry);
	return 0;
}

int shmem_remove_entry(struct shmem_struct *shmem, uintptr_t addr)
{
	return shmem_insert_entry(shmem, addr, 0);
}
