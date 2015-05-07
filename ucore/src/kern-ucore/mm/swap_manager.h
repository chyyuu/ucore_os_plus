#ifndef  _KERN_MM_SWAP_MANAGER_H
#define _KERN_MM_SWAP_MANAGER_H

#include <types.h>
#include <memlayout.h>
#include <vmm.h>
#include <proc.h>

extern unsigned short *mem_map;

#define SWAP_UNUSED                     0xFFFF
#define MAX_SWAP_REF                    0xFFFE

#define HASH_SHIFT                      10
#define HASH_LIST_SIZE                  (1 << HASH_SHIFT)
#define entry_hashfn(x)                 (hash32(x, HASH_SHIFT))

extern wait_queue_t kswapd_done;
extern semaphore_t swap_in_sem;
extern size_t max_swap_offset; 
 // function for swap method
struct Page *swap_hash_find(swap_entry_t entry);
bool swap_page_add(struct Page *page, swap_entry_t entry);
void swap_page_del(struct Page *page);
void swap_free_page(struct Page *page);
void swap_list_del(struct Page* page);
swap_entry_t try_alloc_swap_entry(void);
bool try_free_swap_entry(swap_entry_t entry);

void kswapd_wakeup_all(void);

extern list_entry_t swap_hash_list[HASH_LIST_SIZE];

#define MAX_SWAP_OFFSET_LIMIT                   (1 << 24)
#define swap_offset(entry) ({                                       \
            size_t __offset = (entry >> 8);                         \
            if (!(__offset > 0 && __offset < max_swap_offset)) {    \
                panic("invalid swap_entry_t = %08x.\n", entry);     \
            }                                                       \
            __offset;                                               \
        })

struct swap_manager
{
    const char *name;
    /* Global initialization for the swap manager */
    int (*init)    (void);
    /* Initialize the priv data inside mm_struct */
    int (*init_mm)    (struct mm_struct *mm);
    /* Called when tick interrupt occured */
    int (*tick_event) (void* arg);
    /*remove a swap entry*/
    void (*swap_remove_entry)(swap_entry_t entry);
    /*delete page from swap list*/
    void (*swap_list_del)(struct Page*);
    /* Try to swap out a page, return then victim */
    bool (*swap_out_victim) (size_t n);
    /* check the page relpacement algorithm */
    int (*check_swap)(void);
    /* swap in page from swap space*/
    int (*swap_in_page)(swap_entry_t, struct Page**);
};

// function for using swap method
void swap_manager_init(void);
void swap_tick_event(void* arg);
bool try_free_pages(size_t n);

int swap_page_count(struct Page *page);
void swap_duplicate(swap_entry_t entry);
void swap_remove_entry(swap_entry_t);
//int swap_copy_entry(swap_entry_t, swap_entry_t*);    
int swap_in_page(swap_entry_t, struct Page** pagep);
void def_check_swap();
extern struct swap_manager *  def_swap_manager;

#endif
