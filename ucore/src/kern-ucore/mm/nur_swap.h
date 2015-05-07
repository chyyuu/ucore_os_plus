#ifndef __KERN_MM_NUR_SWAP_H__
#define __KERN_MM_NUR_SWAP_H__

#include <types.h>
#include <memlayout.h>
#include <swap_manager.h>

#ifdef UCONFIG_SWAP
/* *
 * swap_entry_t
 * --------------------------------------------
 * |         offset        |   reserved   | 0 |
 * --------------------------------------------
 *           24 bits            7 bits    1 bit
 * */

extern size_t max_swap_offset;

/* *
 * swap_offset - takes a swap_entry (saved in pte), and returns
 * the corresponding offset in swap mem_map.
 * */

//void swap_init(void);
//bool try_free_pages(size_t n);

//int kswapd_main(void *arg) __attribute__ ((noreturn));

extern  struct swap_manager nur_swap_manager;

#endif /*  UCONFIG_SWAP  */

#endif /* !__KERN_MM_SWAP_H__ */
