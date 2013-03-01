#include <pmm.h>
#include <list.h>
#include <string.h>
#include <buddy_pmm.h>

/* The buddy memory allocation technique is a memory allocation algorithm that divides memory into partitions 
   to try to satisfy a memory request as suitably as possible. This system makes use of splitting memory into halves
   to try to give a best-fit. 
   
   The "Buddy System" algorithm in ucore is drived from:
   1 Page 203~206, Section 8.4 of Yan Wei Ming's chinese book "Data Structure -- C programming language"
   2 Algorithm R & S of section 2.5 of Volume 1 of Knuth's "The Art of Computer Programming", with note taken 
     of exercises 25 and 29 of that section.
   
   In a buddy system, the entire memory space available for allocation is initially treated as a single block whose size
   is a power of 2. When the first request is made, if its size is greater than half of the initial block then the entire
   block is allocated. Otherwise, the block is split in two equal companion buddies. If the size of the request is greater
   than half of one of the buddies, then allocate one to it. Otherwise, one of the buddies is split in half again. This 
   method continues until the smallest block greater than or equal to the size of the request is found and allocated to it.
   
   In this method, when a process terminates the buddy block that was allocated to it is freed. Whenever possible, an 
   unnallocated buddy is merged with a companion buddy in order to form a larger free block. Two blocks are said to be 
   companion buddies if they resulted from the split of the same direct parent block. 
*/

// {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024}
// from 2^0 ~ 2^10
#define MAX_ORDER 10
static free_area_t free_area[MAX_ORDER + 1];

//x from 0 ~ MAX_ORDER
#define free_list(x) (free_area[x].free_list)
#define nr_free(x) (free_area[x].nr_free)

#define MAX_ZONE_NUM 10
struct Zone {
	struct Page *mem_base;
} zones[MAX_ZONE_NUM] = { {
NULL}};

//buddy_init - init the free_list(0 ~ MAX_ORDER) & reset nr_free(0 ~ MAX_ORDER)
static void buddy_init(void)
{
	int i;
	for (i = 0; i <= MAX_ORDER; i++) {
		list_init(&free_list(i));
		nr_free(i) = 0;
	}
}

//buddy_init_memmap - build free_list for Page base follow  n continuous pages.
static void buddy_init_memmap(struct Page *base, size_t n)
{
	static int zone_num = 0;
	assert(n > 0 && zone_num < MAX_ZONE_NUM);
	struct Page *p = base;
	for (; p != base + n; p++) {
		assert(PageReserved(p));
		p->flags = p->property = 0;
		p->zone_num = zone_num;
		set_page_ref(p, 0);
	}
	p = zones[zone_num++].mem_base = base;
	size_t order = MAX_ORDER, order_size = (1 << order);
	while (n != 0) {
		while (n >= order_size) {
			p->property = order;
			SetPageProperty(p);
			list_add(&free_list(order), &(p->page_link));
			n -= order_size, p += order_size;
			nr_free(order)++;
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
static inline struct Page *buddy_alloc_pages_sub(size_t order)
{
	assert(order <= MAX_ORDER);
	size_t cur_order;
	for (cur_order = order; cur_order <= MAX_ORDER; cur_order++) {
		if (!list_empty(&free_list(cur_order))) {
			list_entry_t *le = list_next(&free_list(cur_order));
			struct Page *page = le2page(le, page_link);
			nr_free(cur_order)--;
			list_del(le);
			size_t size = 1 << cur_order;
			while (cur_order > order) {
				cur_order--;
				size >>= 1;
				struct Page *buddy = page + size;
				buddy->property = cur_order;
				SetPageProperty(buddy);
				nr_free(cur_order)++;
				list_add(&free_list(cur_order),
					 &(buddy->page_link));
			}
			ClearPageProperty(page);
			return page;
		}
	}
	return NULL;
}

//buddy_alloc_pages - call buddy_alloc_pages_sub to alloc 2^order>=n pages
static struct Page *buddy_alloc_pages(size_t n)
{
	assert(n > 0);
	size_t order = getorder(n), order_size = (1 << order);
	struct Page *page = buddy_alloc_pages_sub(order);
	if (page != NULL && n != order_size) {
		free_pages(page + n, order_size - n);
	}
	return page;
}

//page_is_buddy - Does this page belong to the No. zone_num Zone & this page
//              -  be in the continuous page block whose size is 2^order pages?
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

//page2idx - get the related index number idx of continuous page block which this page belongs to 
static inline ppn_t page2idx(struct Page *page)
{
	return page - zones[page->zone_num].mem_base;
}

//idx2page - get the related page according to the index number idx of continuous page block 
static inline struct Page *idx2page(int zone_num, ppn_t idx)
{
	return zones[zone_num].mem_base + idx;
}

//buddy_free_pages_sub - the actual free implimentation, should consider how to 
//                     - merge the adjacent buddy block
static void buddy_free_pages_sub(struct Page *base, size_t order)
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
	while (order < MAX_ORDER) {
		buddy_idx = page_idx ^ (1 << order);
		struct Page *buddy = idx2page(zone_num, buddy_idx);
		if (!page_is_buddy(buddy, order, zone_num)) {
			break;
		}
		nr_free(order)--;
		list_del(&(buddy->page_link));
		ClearPageProperty(buddy);
		page_idx &= buddy_idx;
		order++;
	}
	struct Page *page = idx2page(zone_num, page_idx);
	page->property = order;
	SetPageProperty(page);
	nr_free(order)++;
	list_add(&free_list(order), &(page->page_link));
}

//buddy_free_pages - call buddy_free_pages_sub to free n continuous page block
static void buddy_free_pages(struct Page *base, size_t n)
{
	assert(n > 0);
	if (n == 1) {
		buddy_free_pages_sub(base, 0);
	} else {
		size_t order = 0, order_size = 1;
		while (n >= order_size) {
			assert(order <= MAX_ORDER);
			if ((page2idx(base) & order_size) != 0) {
				buddy_free_pages_sub(base, order);
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
			buddy_free_pages_sub(base, order);
			base += order_size;
			n -= order_size;
		}
	}
}

//buddy_nr_free_pages - get the nr: the number of free pages
static size_t buddy_nr_free_pages(void)
{
	size_t ret = 0, order = 0;
	for (; order <= MAX_ORDER; order++) {
		ret += nr_free(order) * (1 << order);
	}
	return ret;
}

//buddy_check - check the correctness of buddy system
static void buddy_check(void)
{
	int i;
	int count = 0, total = 0;
	for (i = 0; i <= MAX_ORDER; i++) {
		list_entry_t *list = &free_list(i), *le = list;
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
		free_lists_store[i] = free_list(i);
		list_init(&free_list(i));
		assert(list_empty(&free_list(i)));
		nr_free_store[i] = nr_free(i);
		nr_free(i) = 0;
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
		free_list(i) = free_lists_store[i];
		nr_free(i) = nr_free_store[i];
	}

	free_pages(p0, 8);
	free_pages(buddy, 8);

	assert(total == nr_free_pages());

	for (i = 0; i <= MAX_ORDER; i++) {
		list_entry_t *list = &free_list(i), *le = list;
		while ((le = list_next(le)) != list) {
			struct Page *p = le2page(le, page_link);
			assert(PageProperty(p) && p->property == i);
			count--, total -= (1 << i);
		}
	}
	assert(count == 0);
	assert(total == 0);
}

//the buddy system pmm
const struct pmm_manager buddy_pmm_manager = {
	.name = "buddy_pmm_manager",
	.init = buddy_init,
	.init_memmap = buddy_init_memmap,
	.alloc_pages = buddy_alloc_pages,
	.free_pages = buddy_free_pages,
	.nr_free_pages = buddy_nr_free_pages,
	.check = buddy_check,
};
