#include <pmm.h>
#include <list.h>
#include <string.h>
#include <buddy_pmm.h>
#include <stdio.h>
#include <sysconf.h>
#include <mp.h>
#include <spinlock.h>
/* XXX struct page may contain race condition?? */

struct numa_mem_zone;
/* free_area_t - maintains a doubly linked list to record free (unused) pages */
typedef struct {
	list_entry_t free_list;	// the list header
	unsigned int nr_free;	// # of free pages in this free list
	struct numa_mem_zone *zone;
} free_area_t;

// {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024}
// from 2^0 ~ 2^10
#define MAX_ORDER 10
/* XXX contented lock? */
static spinlock_s fa_lock[MAX_NUMA_NODES];
static free_area_t free_area[MAX_NUMA_NODES][MAX_ORDER + 1];
static int buddy_numa_borrow = 1;

//x from 0 ~ MAX_ORDER
#define free_list(n,x) (free_area[n][x].free_list)
#define nr_free(n,x) (free_area[n][x].nr_free)

#if 0
#define MAX_ZONE_NUM 10
struct Zone {
	struct Page *mem_base;
} zones[MAX_ZONE_NUM] = { {
NULL}};
#endif

//buddy_init - init the free_list(0 ~ MAX_ORDER) & reset nr_free(0 ~ MAX_ORDER)
static void buddy_init(void)
{
	int i, n;
	for (n = 0; n < MAX_NUMA_NODES; n++){
		for (i = 0; i <= MAX_ORDER; i++) {
			list_init(&free_list(n,i));
			nr_free(n,i) = 0;
		}
		spinlock_init(&fa_lock[n]);
	}
}

//buddy_init_memmap - build free_list for Page base follow  n continuing pages.
static void buddy_init_memmap(struct numa_mem_zone *zone)
{
	static int zone_num = 0;
	size_t n = zone->n;
	assert(n > 0 && zone_num < MAX_NUMA_MEM_ZONES);
	zone_num ++;
	struct Page *base = zone->page;
	struct Page *p = base;
	for (; p != base + n; p++) {
		assert(PageReserved(p));
		p->flags = p->property = 0;
		p->zone_num = zone->id;
		set_page_ref(p, 0);
	}
	//p = zones[zone_num++].mem_base = base;
	p = base;
	size_t order = MAX_ORDER, order_size = (1 << order);
	uint32_t numa_id = zone->node->id;
	assert(numa_id < sysconf.lnuma_count);
	while (n != 0) {
		while (n >= order_size) {
			p->property = order;
			SetPageProperty(p);
			list_add(&free_list(numa_id, order), &(p->page_link));
			n -= order_size, p += order_size;
			nr_free(numa_id, order)++;
		}
		order--;
		order_size >>= 1;
	}
}

//getorder - return order, the minmal 2^order >= n
static inline size_t getorder(size_t n)
{
	size_t order, order_size;
	for (order = 0, order_size = 1; order <= MAX_ORDER;
	     order++, order_size <<= 1) {
		if (n <= order_size) {
			return order;
		}
	}
	panic("getorder failed. %d\n", n);
}

//buddy_alloc_pages_sub - the actual allocation implimentation, return a page whose size >=n,
//                      - the remaining free parts insert to other free list
static inline struct Page *buddy_alloc_pages_sub(uint32_t numa_id, size_t order)
{
	assert(order <= MAX_ORDER);
	size_t cur_order;

	int intr_flag;
	spin_lock_irqsave(&fa_lock[numa_id], intr_flag);
	for (cur_order = order; cur_order <= MAX_ORDER; cur_order++) {
		if (!list_empty(&free_list(numa_id, cur_order))) {
			list_entry_t *le = list_next(&free_list(numa_id, cur_order));
			struct Page *page = le2page(le, page_link);
			nr_free(numa_id, cur_order)--;
			list_del(le);
			size_t size = 1 << cur_order;
			while (cur_order > order) {
				cur_order--;
				size >>= 1;
				struct Page *buddy = page + size;
				buddy->property = cur_order;
				SetPageProperty(buddy);
				nr_free(numa_id, cur_order)++;
				list_add(&free_list(numa_id, cur_order),
					 &(buddy->page_link));
			}
			ClearPageProperty(page);
			spin_unlock_irqrestore(&fa_lock[numa_id], intr_flag);
			return page;
		}
	}
	spin_unlock_irqrestore(&fa_lock[numa_id], intr_flag);
	return NULL;
}

static struct Page *__buddy_alloc_pages_numa(uint32_t numa_id, size_t n)
{
	size_t order = getorder(n), order_size = (1 << order);
	struct Page *page = buddy_alloc_pages_sub(numa_id, order);
	if (page != NULL && n != order_size) {
		free_pages(page + n, order_size - n);
	}
	return page;
}

static struct Page *buddy_alloc_pages_numa(struct numa_node *node, size_t n)
{
	assert(n > 0 && node!=NULL);
	return __buddy_alloc_pages_numa(node->id, n);
}


//buddy_alloc_pages - call buddy_alloc_pages_sub to alloc 2^order>=n pages
static struct Page *buddy_alloc_pages(size_t n)
{
	int i;
	assert(n > 0);
	size_t order = getorder(n), order_size = (1 << order);
	uint32_t numa_id = 0;
	if(mycpu()->node)
		numa_id = mycpu()->node->id;
	struct Page *page = __buddy_alloc_pages_numa(numa_id, n);
	if(page)
		return page;
	if(!buddy_numa_borrow)
		return NULL;
	for(i=0;i<sysconf.lnuma_count;i++){
		if(i == numa_id)
			continue;
		page = __buddy_alloc_pages_numa(i, n);
		if(page){
			kprintf("warning: cpu%d borrow page from node %d\n", myid(), i);
			return page;
		}
	}
	return NULL;
}

//page_is_buddy - Does this page belong to the No. zone_num Zone & this page
//              -  be in the continuing page block whose size is 2^order pages?
static inline bool page_is_buddy(struct Page *page, size_t order, int zone_num)
{
	if (page2ppn(page) < npage) {
		if (page->zone_num == zone_num) {
			return !PageReserved(page) && PageProperty(page)
			    && page->property == order;
		}
	}
	return 0;
}

//page2idx - get the related index number idx of continuing page block which this page belongs to 
static inline ppn_t page2idx(struct Page *page)
{
	return page - numa_mem_zones[page->zone_num].page;
}

//idx2page - get the related page according to the index number idx of continuing page block 
static inline struct Page *idx2page(int zone_num, ppn_t idx)
{
	return numa_mem_zones[zone_num].page + idx;
}

//buddy_free_pages_sub - the actual free implimentation, should consider how to 
//                     - merge the adjacent buddy block
static void buddy_free_pages_sub(uint32_t numa_id, struct Page *base, size_t order)
{
	ppn_t buddy_idx, page_idx = page2idx(base);
	assert((page_idx & ((1 << order) - 1)) == 0);
	struct Page *p = base;
	for (; p != base + (1 << order); p++) {
		assert(!PageReserved(p) && !PageProperty(p));
		p->flags = 0;
		set_page_ref(p, 0);
	}
	int zone_num = base->zone_num;
	int intr_flag;
	while (order < MAX_ORDER) {
		buddy_idx = page_idx ^ (1 << order);
		struct Page *buddy = idx2page(zone_num, buddy_idx);
		if (!page_is_buddy(buddy, order, zone_num)) {
			break;
		}
		/* modify free_list, should lock first */
		spin_lock_irqsave(&fa_lock[numa_id], intr_flag);
		nr_free(numa_id, order)--;
		list_del(&(buddy->page_link));
		spin_unlock_irqrestore(&fa_lock[numa_id], intr_flag);

		ClearPageProperty(buddy);
		page_idx &= buddy_idx;
		order++;
	}
	struct Page *page = idx2page(zone_num, page_idx);
	page->property = order;
	SetPageProperty(page);
	spin_lock_irqsave(&fa_lock[numa_id], intr_flag);
	nr_free(numa_id, order)++;
	list_add(&free_list(numa_id, order), &(page->page_link));
	spin_unlock_irqrestore(&fa_lock[numa_id], intr_flag);
}

//buddy_free_pages - call buddy_free_pages_sub to free n continuing page block
static void buddy_free_pages(struct Page *base, size_t n)
{
	assert(n > 0);
	uint32_t numa_id = numa_mem_zones[base->zone_num].node->id;
	assert(numa_id < sysconf.lnuma_count);
	if (n == 1) {
		buddy_free_pages_sub(numa_id, base, 0);
	} else {
		size_t order = 0, order_size = 1;
		while (n >= order_size) {
			assert(order <= MAX_ORDER);
			if ((page2idx(base) & order_size) != 0) {
				buddy_free_pages_sub(numa_id, base, order);
				base += order_size;
				n -= order_size;
			}
			order++;
			order_size <<= 1;
		}
		while (n != 0) {
			while (n < order_size) {
				order--;
				order_size >>= 1;
			}
			buddy_free_pages_sub(numa_id, base, order);
			base += order_size;
			n -= order_size;
		}
	}
}

//buddy_nr_free_pages - get the nr: the number of free pages
static size_t __buddy_nr_free_pages(uint32_t numa_id)
{
	size_t ret = 0, order = 0;
	int intr_flag;
	spin_lock_irqsave(&fa_lock[numa_id], intr_flag);
	for (; order <= MAX_ORDER; order++) {
		ret += nr_free(numa_id, order) * (1 << order);
	}
	spin_unlock_irqrestore(&fa_lock[numa_id], intr_flag);
	return ret;
}

static size_t buddy_nr_free_pages_numa(struct numa_node* node){
	assert(node != NULL);
	return __buddy_nr_free_pages(node->id);
}

static size_t buddy_nr_free_pages()
{
	int i;
	size_t s = 0;
	for(i=0;i<sysconf.lnuma_count;i++)
		s += __buddy_nr_free_pages(i);
	return s;
}

static void buddy_check_numa(void)
{

}

//buddy_check - check the correctness of buddy system
static void buddy_check(void)
{
#define nr_free_pages() __buddy_nr_free_pages(numa_id)
	uint32_t numa_id = 0;
	int can_borrow = buddy_numa_borrow;
	buddy_numa_borrow = 0;
	int i;
	int count = 0, total = 0;
	for (i = 0; i <= MAX_ORDER; i++) {
		list_entry_t *list = &free_list(numa_id, i), *le = list;
		while ((le = list_next(le)) != list) {
			struct Page *p = le2page(le, page_link);
			assert(PageProperty(p) && p->property == i);
			count++, total += (1 << i);
		}
	}
	assert(total == nr_free_pages());

	struct Page *p0 = alloc_pages(8), *buddy = alloc_pages(8), *p1;

	assert(p0 != NULL);
	assert((page2idx(p0) & 7) == 0);
	assert(!PageProperty(p0));

	list_entry_t free_lists_store[MAX_ORDER + 1];
	unsigned int nr_free_store[MAX_ORDER + 1];

	for (i = 0; i <= MAX_ORDER; i++) {
		free_lists_store[i] = free_list(numa_id, i);
		list_init(&free_list(numa_id, i));
		assert(list_empty(&free_list(numa_id, i)));
		nr_free_store[i] = nr_free(numa_id, i);
		nr_free(numa_id, i) = 0;
	}

	assert(nr_free_pages() == 0);
	assert(alloc_page() == NULL);
	free_pages(p0, 8);
	assert(nr_free_pages() == 8);
	assert(PageProperty(p0) && p0->property == 3);
	assert((p0 = alloc_pages(6)) != NULL && !PageProperty(p0)
	       && nr_free_pages() == 2);

	assert((p1 = alloc_pages(2)) != NULL && p1 == p0 + 6);
	assert(nr_free_pages() == 0);

	free_pages(p0, 3);
	assert(PageProperty(p0) && p0->property == 1);
	assert(PageProperty(p0 + 2) && p0[2].property == 0);

	free_pages(p0 + 3, 3);
	free_pages(p1, 2);

	assert(PageProperty(p0) && p0->property == 3);

	assert((p0 = alloc_pages(6)) != NULL);
	assert((p1 = alloc_pages(2)) != NULL);
	free_pages(p0 + 4, 2);
	free_pages(p1, 2);

	p1 = p0 + 4;
	assert(PageProperty(p1) && p1->property == 2);
	free_pages(p0, 4);
	assert(PageProperty(p0) && p0->property == 3);

	assert((p0 = alloc_pages(8)) != NULL);
	assert(alloc_page() == NULL && nr_free_pages() == 0);

	for (i = 0; i <= MAX_ORDER; i++) {
		free_list(numa_id, i) = free_lists_store[i];
		nr_free(numa_id, i) = nr_free_store[i];
	}

	free_pages(p0, 8);
	free_pages(buddy, 8);

	assert(total == nr_free_pages());

	for (i = 0; i <= MAX_ORDER; i++) {
		list_entry_t *list = &free_list(numa_id, i), *le = list;
		while ((le = list_next(le)) != list) {
			struct Page *p = le2page(le, page_link);
			assert(PageProperty(p) && p->property == i);
			count--, total -= (1 << i);
		}
	}
	assert(count == 0);
	assert(total == 0);
	buddy_numa_borrow = can_borrow;
#undef nr_free_pages
}

//the buddy system pmm
const struct pmm_manager buddy_pmm_manager = {
	.name = "buddy_pmm_manager",
	.init = buddy_init,
	.init_memmap = buddy_init_memmap,
	.alloc_pages = buddy_alloc_pages,
	.alloc_pages_numa = buddy_alloc_pages_numa,
	.free_pages = buddy_free_pages,
	.nr_free_pages = buddy_nr_free_pages,
	.nr_free_pages_numa = buddy_nr_free_pages_numa,
	.check = buddy_check,
};

