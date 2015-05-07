#include "fifo_swap.h"
#include <vmm.h>
#include <pmm.h>
#include <types.h>
#include <memlayout.h>
#include <swap_manager.h>

extern unsigned short* mem_map;
static bool swap_init_ok = 0;

struct vma_struct *current_vma = NULL;

int pressure = 0;

int fifo_swap_in_page(swap_entry_t entry, struct Page** pagep){
    if (pagep == NULL){
        return -E_INVAL;
    }
    size_t offset = swap_offset(entry);
    assert(mem_map[offset] >= 0);
    int ret;

    struct Page* page, *newpage;
    if ((page = swap_hash_find(entry)) != NULL){
        goto found;
    }
    newpage = alloc_page();
    down(&swap_in_sem);
    if ((page = swap_hash_find(entry)) != NULL) {
        if (newpage != NULL) {
            free_page(newpage);
        }
        goto found_unlock;
    }
    if (newpage == NULL) {
        ret = -E_NO_MEM;
        goto failed_unlock;
    }
    page = newpage;
    if (swapfs_read(entry, page) != 0) {
        free_page(page);
        ret = -E_SWAP_FAULT;
        goto failed_unlock;
    }
    swap_page_add(page, entry);
found_unlock:
    up(&swap_in_sem);
found:
    *pagep = page;
    return 0;

failed_unlock:
    up(&swap_in_sem);
    return ret;
}

int swap_out_vma(struct mm_struct* mm, struct vma_struct* vma, uintptr_t addr, int needs){  
    uintptr_t end;
    size_t free_count = 0;
    addr = ROUNDDOWN(addr, PGSIZE); 
    end = ROUNDUP(vma->vm_end, PGSIZE);

    while (addr < end && needs != 0){
        pte_t * ptep = get_pte(mm->pgdir, addr, 0);
        if (ptep == NULL){
            addr += PGSIZE;
            continue;
        }

        if (ptep_present(ptep)){            
                    struct Page *page = pte2page(*ptep);
                    assert(!PageReserved(page));
                    if(!PageSwap(page)){
                        if (!swap_page_add(page, 0)){
                            kprintf("swap page add failed\n");
                            goto try_next_entry;
                        }
                    }
                    swap_entry_t entry = page->index;
                    swap_duplicate(entry);
                    page_ref_dec(page);
                    ptep_copy(ptep, &entry);
                    mp_tlb_invalidate(mm->pgdir, addr);

                    if ((vma->vm_flags & VM_SHARE) && page_ref(page) == 1) {
                        uintptr_t shmem_addr =
                            addr - vma->vm_start + vma->shmem_off;
                        pte_t *sh_ptep =
                            shmem_get_entry(vma->shmem, shmem_addr, 0);
                        assert(sh_ptep != NULL
                               && !ptep_invalid(sh_ptep));
                        if (ptep_present(sh_ptep)) {
                            shmem_insert_entry(vma->shmem,
                                       shmem_addr, entry);
                        }
                    }
                    if (page_ref(page) == 0){  
                       swap_duplicate(entry);
                       if (swapfs_write(entry, page) !=0){
                            mem_map[swap_offset(entry)]--;
                            goto try_next_entry;
                       }
                       mem_map[swap_offset(entry)]--;
                       swap_free_page(page);
                       free_count++, needs--;
                    }
        }
try_next_entry:
        addr += PGSIZE;
    }
    return free_count;
}

int swap_out_mm(struct mm_struct* mm, int needs){
    if (mm ==NULL || needs == 0 || mm->map_count == 0){
        return 0;
    }
    assert(!list_empty(&(mm->mmap_list)));
    if (current_vma == NULL){
        current_vma = le2vma(list_next(&(mm->mmap_list)), list_link);
    }
    uintptr_t addr = current_vma->vm_start;

    int i;
    size_t free_count = 0;
    for (i=0; i<mm->map_count; i++){
        int ret = swap_out_vma(mm, current_vma, addr, needs);
        free_count += ret; 
        needs -= ret;

        list_entry_t* le = list_next(&(current_vma->list_link));
        if (le == &(mm->mmap_list)){
            current_vma = NULL;
            break;
        }
        current_vma = le2vma(le, list_link);
        if (needs == 0){
            break;
        }
        addr = current_vma->vm_start;
    }   
    return free_count;
}

void fifo_tick_event(){
    while(1){
        int guard = 0;
        if (pressure > 0){
            int round=16;
            list_entry_t * list = &proc_mm_list;
            assert(!list_empty(list));
            while (pressure > 0 && round-- >0){
                list_entry_t* le=list_next(list);
                list_del(le);
                list_add_before(list,le);
                struct mm_struct* mm = le2mm(le, proc_mm_link);
                pressure -= swap_out_mm(mm, pressure);
            }
        }
        if (pressure> 0){
            if (guard++ >= 1000){
                warn("fifo tick event: may out of memory\n");
                break;
            }
            continue;
        }
        break;
    }    
    pressure = 0;
    kswapd_wakeup_all();
}

void fifo_init(){
    def_check_swap();
    swap_init_ok = 1;
}

void fifo_swap_list_del(struct Page* page){
    assert(PageSwap(page));
    list_del(&(page->swap_link));
}

void fifo_swap_remove_entry(swap_entry_t entry){
    size_t offset= swap_offset(entry);
    assert(mem_map[offset] >0);
    if (--mem_map[offset] == 0){
        struct Page* page= swap_hash_find(entry);
        if (page != NULL){
            if (page_ref(page) != 0){
                return ;
            }
            fifo_swap_list_del(page);
            swap_free_page(page);
        }
        mem_map[offset] = SWAP_UNUSED;
    }
}

bool fifo_swap_out(int needs){
    if (!swap_init_ok || kswapd == NULL){
        return 0;
    }
    if (current == kswapd){
        panic("kswapd call try free pages");
    }
    pressure += needs << 5;
    wait_t __wait, *wait = &__wait;

    bool intr_flag;
    local_intr_save(intr_flag);
    {
        wait_init(wait, current);
        current->state = PROC_SLEEPING;
        current->wait_state = WT_KSWAPD;
        wait_queue_add(&kswapd_done, wait);
        if (kswapd->wait_state == WT_TIMER) {
            wakeup_proc(kswapd);
        }
    }
    local_intr_restore(intr_flag);
    schedule();
    assert(!wait_in_queue(wait) && wait->wakeup_flags == WT_KSWAPD);
    return 1;
}

void fifo_check_swap(){
    kprintf("I am fifo check_swap \n");

    struct mm_struct * mm = mm_create();
    pgd_t* pgdir = mm->pgdir = alloc_page();
    struct vma_struct *vma=
        vma_create(TEST_PAGE, TEST_PAGE+PTSIZE, VM_WRITE|VM_READ);

    insert_vma_struct(mm, vma);
    struct Page *rp0 = alloc_page();
    assert(rp0 != NULL );

    pte_perm_t perm;
    ptep_unmap(&perm);
    ptep_set_u_write(&perm);
    int ret = page_insert(pgdir, rp0, TEST_PAGE, perm);
    assert(ret == 0 && page_ref(rp0) == 1);

    pte_t* ptep = get_pte(pgdir, TEST_PAGE, 0);
    //kprintf("ptep: 0x%x\n", *ptep);
    int i;
    for (i=0; i<PGSIZE; i++){
        ((char*)page2kva(rp0))[i] = (char)i;
    }
    swap_out_mm(mm, 1);

    swap_entry_t entry = *get_pte(pgdir, TEST_PAGE, 0);
    //kprintf("entry:0x%x \n",entry);
    struct Page* newpage = NULL;
    assert(swap_in_page(entry, &newpage) == 0);

    for (i=0; i<PGSIZE; i++){
        assert(((char*)page2kva(newpage))[i] == (char)i);
    }
    kprintf("swap in successed\n");

    swap_free_page(newpage);
    mm_destroy(mm);
    
    kprintf("fifo check_swap successed\n");
}
struct swap_manager fifo_swap_manager = {
    .name = "first in first out",
    .init = fifo_init,
    .init_mm = NULL,
    .tick_event = fifo_tick_event,
    .swap_remove_entry = fifo_swap_remove_entry,
    .swap_list_del = fifo_swap_list_del,
    .swap_in_page = fifo_swap_in_page,
    .swap_out_victim = fifo_swap_out,
    .check_swap = fifo_check_swap,
};

