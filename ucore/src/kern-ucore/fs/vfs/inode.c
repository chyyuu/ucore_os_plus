#include <types.h>
#include <stdio.h>
#include <string.h>
#include <atomic.h>
#include <slab.h>
#include <vfs.h>
#include <inode.h>
#include <error.h>
#include <assert.h>
#include <kio.h>

/* *
 * __alloc_inode - alloc a inode structure and initialize in_type
 * */
struct inode *__alloc_inode(int type)
{
	struct inode *node;
	if ((node = kmalloc(sizeof(struct inode))) != NULL) {
		node->in_type = type;
	}
	return node;
}

/* *
 * inode_init - initialize a inode structure
 * invoked by vop_init
 * */
void inode_init(struct inode *node, const struct inode_ops *ops, struct fs *fs)
{
	atomic_set(&(node->ref_count), 0);
	atomic_set(&(node->open_count), 0);
	node->in_ops = ops, node->in_fs = fs;
#ifdef UCONFIG_BIONIC_LIBC
	list_init(&(node->mapped_addr_list));
#endif //UCONFIG_BIONIC_LIBC
	vop_ref_inc(node);
}

/* *
 * inode_kill - kill a inode structure
 * invoked by vop_kill
 * */
void inode_kill(struct inode *node)
{
	assert(inode_ref_count(node) == 0);
	assert(inode_open_count(node) == 0);
	kfree(node);
}

/* *
 * inode_ref_inc - increment ref_count
 * invoked by vop_ref_inc
 * */
int inode_ref_inc(struct inode *node)
{
	return atomic_add_return(&(node->ref_count), 1);
}

/* *
 * inode_ref_dec - decrement ref_count
 * invoked by vop_ref_dec
 * calls vop_reclaim if the ref_count hits zero
 * */
int inode_ref_dec(struct inode *node)
{
	assert(inode_ref_count(node) > 0);
	int ref_count, ret;
	if ((ref_count = atomic_sub_return(&(node->ref_count), 1)) == 0) {
		if ((ret = vop_reclaim(node)) != 0 && ret != -E_BUSY) {
			kprintf("vfs: warning: vop_reclaim: %e.\n", ret);
		}
	}
	return ref_count;
}

/* *
 * inode_open_inc - increment the open_count
 * invoked by vop_open_inc
 * */
int inode_open_inc(struct inode *node)
{
	return atomic_add_return(&(node->open_count), 1);
}

/* *
 * inode_open_dec - decrement the open_count
 * invoked by vop_open_dec
 * calls vop_close if the open_count hits zero
 * */
int inode_open_dec(struct inode *node)
{
	assert(inode_open_count(node) > 0);
	int open_count, ret;
	if ((open_count = atomic_sub_return(&(node->open_count), 1)) == 0) {
		if ((ret = vop_close(node)) != 0) {
			kprintf("vfs: warning: vop_close: %e.\n", ret);
		}
	}
	return open_count;
}

/* *
 * inode_check - check the various things being valid
 * called before all vop_* calls
 * */
void inode_check(struct inode *node, const char *opstr)
{
	assert(node != NULL && node->in_ops != NULL);
	assert(node->in_ops->vop_magic == VOP_MAGIC);
	int ref_count = inode_ref_count(node), open_count =
	    inode_open_count(node);
	assert(ref_count >= open_count && open_count >= 0);
	assert(ref_count < MAX_INODE_COUNT && open_count < MAX_INODE_COUNT);
}

/* *
 * null_vop_* - null vop functions
 * */
int null_vop_pass(void)
{
	return 0;
}

int null_vop_inval(void)
{
	return -E_INVAL;
}

int null_vop_unimp(void)
{
	return -E_UNIMP;
}

int null_vop_isdir(void)
{
	return -E_ISDIR;
}

int null_vop_notdir(void)
{
	return -E_NOTDIR;
}
