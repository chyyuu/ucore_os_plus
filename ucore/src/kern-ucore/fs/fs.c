#include <types.h>
#include <slab.h>
#include <sem.h>
#include <vfs.h>
#include <dev.h>
#include <file.h>
#include <pipe.h>
#include <sfs.h>
#include <inode.h>
#include <assert.h>

void fs_init(void)
{
	vfs_init();
	dev_init();
	pipe_init();
	sfs_init();
}

void fs_cleanup(void)
{
	vfs_unmount_all();
	vfs_cleanup();
}

void lock_fs(struct fs_struct *fs_struct)
{
	down(&(fs_struct->fs_sem));
}

void unlock_fs(struct fs_struct *fs_struct)
{
	up(&(fs_struct->fs_sem));
}

struct fs_struct *fs_create(void)
{
	static_assert((int)FS_STRUCT_NENTRY > 128);
	struct fs_struct *fs_struct;
	if ((fs_struct =
	     kmalloc(sizeof(struct fs_struct) + FS_STRUCT_BUFSIZE)) != NULL) {
		fs_struct->pwd = NULL;
		fs_struct->filemap = (void *)(fs_struct + 1);
		atomic_set(&(fs_struct->fs_count), 0);
		sem_init(&(fs_struct->fs_sem), 1);
		filemap_init(fs_struct->filemap);
	}
	return fs_struct;
}

void fs_destroy(struct fs_struct *fs_struct)
{
	assert(fs_struct != NULL && fs_count(fs_struct) == 0);
	if (fs_struct->pwd != NULL) {
		vop_ref_dec(fs_struct->pwd);
	}
	int i;
	struct file *file = fs_struct->filemap;
	for (i = 0; i < FS_STRUCT_NENTRY; i++, file++) {
		if (file->status == FD_OPENED) {
			filemap_close(file);
		}
		if (file->status != FD_NONE)
			kprintf("file->fd:%d\n", file->fd);
		assert(file->status == FD_NONE);
	}
	kfree(fs_struct);
}

void fs_closeall(struct fs_struct *fs_struct)
{
	assert(fs_struct != NULL && fs_count(fs_struct) > 0);
	int i;
	struct file *file = fs_struct->filemap;
	for (i = 3, file += 2; i < FS_STRUCT_NENTRY; i++, file++) {
		if (file->status == FD_OPENED) {
			filemap_close(file);
		}
	}
}

int dup_fs(struct fs_struct *to, struct fs_struct *from)
{
	assert(to != NULL && from != NULL);
	assert(fs_count(to) == 0 && fs_count(from) > 0);
	if ((to->pwd = from->pwd) != NULL) {
		vop_ref_inc(to->pwd);
	}
	int i;
	struct file *to_file = to->filemap, *from_file = from->filemap;
	for (i = 0; i < FS_STRUCT_NENTRY; i++, to_file++, from_file++) {
		if (from_file->status == FD_OPENED) {
			/* alloc_fd first */
			to_file->status = FD_INIT;
			filemap_dup(to_file, from_file);
		}
		else if(from_file->status != FD_NONE) {
			to_file->status = from_file->status;
			filemap_dup_close(to_file, from_file);
		}
	}
	return 0;
}
