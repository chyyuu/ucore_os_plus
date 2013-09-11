#ifndef __KERN_MM_VMM_H__
#define __KERN_MM_VMM_H__

#ifndef __LINUX_GLUE__
#include <types.h>
#include <list.h>
#include <memlayout.h>
#include <rb_tree.h>
#include <sync.h>
#include <shmem.h>
#include <atomic.h>
#include <sem.h>
#endif

//pre define
struct mm_struct;

#ifdef UCONFIG_BIONIC_LIBC
struct mapped_file_struct {
	struct file *file;
	off_t offset;
};
#endif //UCONFIG_BIONIC_LIBC

// the virtual continuous memory area(vma), [vm_start, vm_end),
// addr belong to a vma means  vma.vm_start<= addr <vma.vm_end
struct vma_struct {
	struct mm_struct *vm_mm; // the set of vma using the same PDT
	uintptr_t vm_start;      // start addr of vma
	uintptr_t vm_end;        // end addr of vma, not include the vm_end itself
	uint32_t vm_flags;	// flags of vma
	rb_node rb_link;	// redblack link which sorted by start addr of vma
	list_entry_t list_link;	// linear list link which sorted by start addr of vma
	struct shmem_struct *shmem;
	size_t shmem_off;
#ifdef UCONFIG_BIONIC_LIBC
	struct mapped_file_struct mfile;
#endif				//UCONFIG_BIONIC_LIBC
};

#define le2vma(le, member)                  \
    to_struct((le), struct vma_struct, member)

#define rbn2vma(node, member)               \
    to_struct((node), struct vma_struct, member)

#define VM_READ                 0x00000001
#define VM_WRITE                0x00000002
#define VM_EXEC                 0x00000004
#define VM_STACK                0x00000008
#define VM_SHARE                0x00000010

#define VM_ANONYMOUS			0x00000020

/* must the same as Linux */
#define VM_IO           0x00004000

#define MAP_FAILED      ((void*)-1)
#define MAP_SHARED      0x01	/* Share changes */
#define MAP_PRIVATE     0x02	/* Changes are private */
#define MAP_TYPE        0x0f	/* Mask for type of mapping */
#define MAP_FIXED       0x10	/* Interpret addr exactly */
#define MAP_ANONYMOUS   0x20	/* don't use a file */

#define MAP_STACK		0x20000

#define PROT_READ       0x1	/* page can be read */
#define PROT_WRITE      0x2	/* page can be written */
#define PROT_EXEC       0x4	/* page can be executed */
#define PROT_SEM        0x8	/* page may be used for atomic ops */
#define PROT_NONE       0x0	/* page can not be accessed */
#define PROT_GROWSDOWN  0x01000000	/* mprotect flag: extend change to start of growsdown vma */
#define PROT_GROWSUP    0x02000000	/* mprotect flag: extend change to end of growsup vma */

struct mm_struct {
	list_entry_t mmap_list;
	rb_tree *mmap_tree;
	struct vma_struct *mmap_cache;
	pgd_t *pgdir;
#ifdef ARCH_ARM
	//ARM PDT is 16K alignment
	//but our allocator do not support 
	//this sort of allocation
	pgd_t *pgdir_alloc_addr;
#endif
	int map_count;
	uintptr_t swap_address;
	atomic_t mm_count;
	int locked_by;
	uintptr_t brk_start, brk;
	list_entry_t proc_mm_link;
	semaphore_t mm_sem;
};

void lock_mm(struct mm_struct *mm);
void unlock_mm(struct mm_struct *mm);
bool try_lock_mm(struct mm_struct *mm);

#define le2mm(le, member)                   \
    to_struct((le), struct mm_struct, member)

#define RB_MIN_MAP_COUNT        32	// If the count of vma >32 then redblack tree link is used

struct vma_struct *find_vma(struct mm_struct *mm, uintptr_t addr);
struct vma_struct *find_vma_intersection(struct mm_struct *mm, uintptr_t start,
					 uintptr_t end);
struct vma_struct *vma_create(uintptr_t vm_start, uintptr_t vm_end,
			      uint32_t vm_flags);
void insert_vma_struct(struct mm_struct *mm, struct vma_struct *vma);

struct mm_struct *mm_create(void);
void mm_destroy(struct mm_struct *mm);

void vmm_init(void);
int mm_map(struct mm_struct *mm, uintptr_t addr, size_t len, uint32_t vm_flags,
	   struct vma_struct **vma_store);
int mm_map_shmem(struct mm_struct *mm, uintptr_t addr, uint32_t vm_flags,
		 struct shmem_struct *shmem, struct vma_struct **vma_store);
int mm_unmap(struct mm_struct *mm, uintptr_t addr, size_t len);
int dup_mmap(struct mm_struct *to, struct mm_struct *from);
void exit_mmap(struct mm_struct *mm);
uintptr_t get_unmapped_area(struct mm_struct *mm, size_t len);
int mm_brk(struct mm_struct *mm, uintptr_t addr, size_t len);

int do_pgfault(struct mm_struct *mm, machine_word_t error_code, uintptr_t addr);
bool user_mem_check(struct mm_struct *mm, uintptr_t start, size_t len,
		    bool write);

size_t user_mem_check_size(struct mm_struct *mm, uintptr_t start,
			   size_t len, bool write);

bool copy_from_user(struct mm_struct *mm, void *dst, const void *src,
		    size_t len, bool writable);
bool copy_to_user(struct mm_struct *mm, void *dst, const void *src, size_t len);
bool copy_string(struct mm_struct *mm, char *dst, const char *src, size_t maxn);

static inline int mm_count(struct mm_struct *mm)
{
	return atomic_read(&(mm->mm_count));
}

static inline void set_mm_count(struct mm_struct *mm, int val)
{
	atomic_set(&(mm->mm_count), val);
}

static inline int mm_count_inc(struct mm_struct *mm)
{
	return atomic_add_return(&(mm->mm_count), 1);
}

static inline int mm_count_dec(struct mm_struct *mm)
{
	return atomic_sub_return(&(mm->mm_count), 1);
}

#endif /* !__KERN_MM_VMM_H__ */
