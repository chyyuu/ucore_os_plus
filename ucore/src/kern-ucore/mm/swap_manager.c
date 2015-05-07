#include <swap_manager.h>
#include <nur_swap.h>
#include <error.h>
#include <pmm.h>
#include <vmm.h>
#include <proc.h>

struct swap_manager * def_swap_manager =NULL;

wait_queue_t kswapd_done;
semaphore_t swap_in_sem;
unsigned short *mem_map;
int swap_time;

void swap_manager_init(void){
    swapfs_init();
    def_swap_manager = &nur_swap_manager;
    sem_init(&swap_in_sem, 1);
    wait_queue_init(&kswapd_done);
    if (!
        (512 <= max_swap_offset
         && max_swap_offset < MAX_SWAP_OFFSET_LIMIT)) {
        panic("bad max_swap_offset %08x.\n", max_swap_offset);
    }

    mem_map = kmalloc(sizeof(short) * max_swap_offset);
    assert(mem_map != NULL);

    size_t offset;
    for (offset = 0; offset < max_swap_offset; offset++) {
        mem_map[offset] = SWAP_UNUSED;
    }

    int i;
    for (i = 0; i < HASH_LIST_SIZE; i++) {
        list_init(swap_hash_list + i);
    }

    swap_time = 0;
    def_swap_manager->init();
    def_swap_manager->check_swap();
}

void swap_tick_event(void *arg){
        while(1){
            def_swap_manager->tick_event(arg);
            kprintf("swap page:%d\n", swap_time);
            do_sleep(1000);
        }
}

void swap_remove_entry(swap_entry_t entry){
    def_swap_manager->swap_remove_entry(entry);
}

/*
int swap_copy_entry(swap_entry_t entry, swap_entry_t* store){
    return def_swap_manager->swap_copy_entry(entry, store);
} */
void kswapd_wakeup_all(void)
{
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        wakeup_queue(&kswapd_done, WT_KSWAPD, 1);
    }
    local_intr_restore(intr_flag);
}

// swap_page_count - get reference number of swap page frame
int swap_page_count(struct Page *page)
{
    if (!PageSwap(page)) {
        return 0;
    }
    size_t offset = swap_offset(page->index);
    assert(mem_map[offset] >= 0);
    return mem_map[offset];
}

// swap_duplicate - reference number of mem_map[offset] ++
void swap_duplicate(swap_entry_t entry)
{
    size_t offset = swap_offset(entry);
    assert(mem_map[offset] >= 0);
    if (mem_map[offset] >= MAX_SWAP_REF){
        int i;
        for (i=0; i<max_swap_offset; i++){
            if (mem_map[i] < MAX_SWAP_REF){
                kprintf("find %d mem_map= 0x%x\n", i, mem_map[i]);
            }
        }
    }
    assert(mem_map[offset] < MAX_SWAP_REF);
    mem_map[offset]++;
}

// the hash list used to find swap page according to swap entry quickly.
list_entry_t swap_hash_list[HASH_LIST_SIZE];

// swap_hash_find - find page according entry using swap hash list
struct Page *swap_hash_find(swap_entry_t entry)
{
    list_entry_t *list = swap_hash_list + entry_hashfn(entry), *le = list;
    while ((le = list_next(le)) != list) {
        struct Page *page = le2page(le, page_link);
        if (page->index == entry) {
            return page;
        }
    }
    return NULL;
}

void swap_list_del(struct Page* page){
    def_swap_manager->swap_list_del(page);
}

// swap_page_add - set PG_swap flag in page, set page->index = entry, and add page to swap_hash_list.
//               - if entry==0, It means ???
bool swap_page_add(struct Page *page, swap_entry_t entry)
{
    assert(!PageSwap(page));
    if (entry == 0) {
        if ((entry = try_alloc_swap_entry()) == 0) {
            return 0;
        }
        assert(mem_map[swap_offset(entry)] == SWAP_UNUSED);
        mem_map[swap_offset(entry)] = 0;
        SetPageDirty(page);
    }
    SetPageSwap(page);
    page->index = entry;
    list_add(swap_hash_list + entry_hashfn(entry), &(page->page_link));
    return 1;
}

// swap_page_del - clear PG_swap flag in page, and del page from swap_hash_list.
void swap_page_del(struct Page *page)
{
    assert(PageSwap(page));
    ClearPageSwap(page);
    list_del(&(page->page_link));
}

// swap_free_page - call swap_page_del&free_page to generate a free page
void swap_free_page(struct Page *page)
{
    assert(PageSwap(page) && page_ref(page) == 0);
    swap_page_del(page);
    free_page(page);
}

// try_alloc_swap_entry - try to alloc a unused swap entry
swap_entry_t try_alloc_swap_entry(void)
{
    static size_t next = 1;
    size_t empty = 0, zero = 0, end = next;
    do {
        switch (mem_map[next]) {
        case SWAP_UNUSED:
            empty = next;
            break;
        case 0:
            if (zero == 0) {
                zero = next;
            }
            break;
        }
        if (++next == max_swap_offset) {
            next = 1;
        }
    } while (empty == 0 && next != end);

    swap_entry_t entry = 0;
    if (empty != 0) {
        entry = (empty << 8);
    } else if (zero != 0) {
        entry = (zero << 8);
        struct Page *page = swap_hash_find(entry);
        assert(page != NULL && PageSwap(page));
        swap_list_del(page);
        if (page_ref(page) == 0) {
            swap_free_page(page);
        } else {
            swap_page_del(page);
        }
        mem_map[zero] = SWAP_UNUSED;
    }

    static unsigned int failed_counter = 0;
    if (entry == 0 && ((++failed_counter) % 0x1000) == 0) {
        warn("swap: try_alloc_swap_entry: failed too many times.\n");
    }
    return entry;
}

// try_free_swap_entry - if mem_map[offset] == 0 (no reference), then mem_map[offset] = SWAP_UNUSED
bool try_free_swap_entry(swap_entry_t entry)
{
    size_t offset = swap_offset(entry);
    if (mem_map[offset] == 0) {
        mem_map[offset] = SWAP_UNUSED;
        return 1;
    }
    return 0;
}

bool try_free_pages(size_t n){
    swap_time += n;
    if (def_swap_manager != NULL)
        return def_swap_manager->swap_out_victim(n);
    else
        return 0;
}

int swap_in_page(swap_entry_t entry, struct Page** pagep){
    return def_swap_manager->swap_in_page(entry, pagep);
}

void def_check_swap(){
    /*
    assert(kloopd->mm != NULL);
    assert(!list_empty(&(kloopd->mm->mmap_list)));
    struct mm_struct *mm = kloopd->mm;
    struct vma_struct *vma = le2vma(list_next(&(mm->mmap_list)), list_link);
    int i;
    kprintf("======= kloopd vma ==========\n");
    for (i=0; i<mm->map_count; i++){
        kprintf("  start:%x \t end:%x \n", vma->vm_start, vma->vm_end);
        vma = le2vma(list_next(&(vma->list_link)), list_link);
    } */
   def_swap_manager->check_swap();
}