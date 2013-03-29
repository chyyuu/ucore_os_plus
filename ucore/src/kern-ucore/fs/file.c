#include <types.h>
#include <string.h>
#include <slab.h>
#include <vfs.h>
#include <proc.h>
#include <file.h>
#include <unistd.h>
#include <iobuf.h>
#include <inode.h>
#include <stat.h>
#include <dirent.h>
#include <error.h>
#include <assert.h>

#include <vmm.h>

#define testfd(fd)                          ((fd) >= 0 && (fd) < FS_STRUCT_NENTRY)

static struct file *get_filemap(void)
{
	struct fs_struct *fs_struct = current->fs_struct;
	assert(fs_struct != NULL && fs_count(fs_struct) > 0);
	return fs_struct->filemap;
}

void filemap_init(struct file *filemap)
{
	int fd;
	struct file *file = filemap;
	for (fd = 0; fd < FS_STRUCT_NENTRY; fd++, file++) {
		atomic_set(&(file->open_count), 0);
		file->status = FD_NONE, file->fd = fd;
	}
}

static int filemap_alloc(int fd, struct file **file_store)
{
	struct file *file = get_filemap();
	if (fd == NO_FD) {
		for (fd = 0; fd < FS_STRUCT_NENTRY; fd++, file++) {
			if (file->status == FD_NONE) {
				goto found;
			}
		}
		return -E_MAX_OPEN;
	} else {
		if (testfd(fd)) {
			file += fd;
			if (file->status == FD_NONE) {
				goto found;
			}
			return -E_BUSY;
		}
		return -E_INVAL;
	}
found:
	assert(fopen_count(file) == 0);
	file->status = FD_INIT, file->node = NULL;
	*file_store = file;
	return 0;
}

static void filemap_free(struct file *file)
{
	assert(file->status == FD_INIT || file->status == FD_CLOSED);
	assert(fopen_count(file) == 0);
	if (file->status == FD_CLOSED) {
		vfs_close(file->node);
	}
	file->status = FD_NONE;
}

void filemap_acquire(struct file *file)
{
	assert(file->status == FD_OPENED || file->status == FD_CLOSED);
	fopen_count_inc(file);
}

void filemap_release(struct file *file)
{
	assert(file->status == FD_OPENED || file->status == FD_CLOSED);
	assert(fopen_count(file) > 0);
	if (fopen_count_dec(file) == 0) {
		filemap_free(file);
	}
}

void filemap_open(struct file *file)
{
	assert((file->status == FD_INIT || file->status == FD_OPENED)
	       && file->node != NULL);
	file->status = FD_OPENED;
	fopen_count_inc(file);
}

void filemap_close(struct file *file)
{
	assert(file->status == FD_OPENED);
	assert(fopen_count(file) > 0);
	file->status = FD_CLOSED;
	if (fopen_count_dec(file) == 0) {
		filemap_free(file);
	}
}

void filemap_dup(struct file *to, struct file *from)
{
	assert(to->status == FD_INIT && from->status == FD_OPENED);
	to->pos = from->pos;
	to->readable = from->readable;
	to->writable = from->writable;
	struct inode *node = from->node;
	vop_ref_inc(node), vop_open_inc(node);
	to->node = node;
	filemap_open(to);
}

void filemap_dup_close(struct file *to, struct file *from)
{
	assert(to->status == FD_CLOSED && from->status == FD_CLOSED);
	to->pos = from->pos;
	to->readable = from->readable;
	to->writable = from->writable;
	struct inode *node = from->node;
	vop_ref_inc(node), vop_open_inc(node);
	to->node = node;
}

static inline int fd2file(int fd, struct file **file_store)
{
	if (testfd(fd)) {
		struct file *file = get_filemap() + fd;
		if (file->status == FD_OPENED && file->fd == fd) {
			*file_store = file;
			return 0;
		}
	}
	return -E_INVAL;
}

#ifdef UCONFIG_BIONIC_LIBC
struct file*
fd2file_onfs(int fd, struct fs_struct *fs_struct) {
	if(testfd(fd)) {
		assert(fs_struct != NULL && fs_count(fs_struct) > 0);
		struct file *file = fs_struct->filemap + fd;
		if((file->status == FD_OPENED || file->status == FD_CLOSED) && file->fd == fd) {
			return file;
		}
	} else {
		panic("testfd() failed");
	}
	return NULL;
}
#endif //UCONFIG_BIONIC_LIBC

bool file_testfd(int fd, bool readable, bool writable)
{
	int ret;
	struct file *file;
	if ((ret = fd2file(fd, &file)) != 0) {
		return 0;
	}
	if (readable && !file->readable) {
		return 0;
	}
	if (writable && !file->writable) {
		return 0;
	}
	return 1;
}

int file_open(char *path, uint32_t open_flags)
{
	bool readable = 0, writable = 0;
	switch (open_flags & O_ACCMODE) {
	case O_RDONLY:
		readable = 1;
		break;
	case O_WRONLY:
		writable = 1;
		break;
	case O_RDWR:
		readable = writable = 1;
		break;
	default:
		return -E_INVAL;
	}

	int ret;
	struct file *file;
	if ((ret = filemap_alloc(NO_FD, &file)) != 0) {
		return ret;
	}

	struct inode *node;
	if ((ret = vfs_open(path, open_flags, &node)) != 0) {
		filemap_free(file);
		return ret;
	}

	file->pos = 0;
	if (open_flags & O_APPEND) {
		struct stat __stat, *stat = &__stat;
		if ((ret = vop_fstat(node, stat)) != 0) {
			vfs_close(node);
			filemap_free(file);
			return ret;
		}
		file->pos = stat->st_size;
	}

	file->node = node;
	file->readable = readable;
	file->writable = writable;
	filemap_open(file);
	return file->fd;
}

int file_close(int fd)
{
	int ret;
	struct file *file;
	if ((ret = fd2file(fd, &file)) != 0) {
		return ret;
	}
	filemap_close(file);
	return 0;
}

int file_read(int fd, void *base, size_t len, size_t * copied_store)
{
	int ret;
	struct file *file;
	*copied_store = 0;
	if ((ret = fd2file(fd, &file)) != 0) {
		return ret;
	}
	if (!file->readable) {
		return -E_INVAL;
	}
	filemap_acquire(file);

	struct iobuf __iob, *iob = iobuf_init(&__iob, base, len, file->pos);
	ret = vop_read(file->node, iob);

	size_t copied = iobuf_used(iob);
	if (file->status == FD_OPENED) {
		file->pos += copied;
	}
	*copied_store = copied;
	filemap_release(file);
	return ret;
}

int file_write(int fd, void *base, size_t len, size_t * copied_store)
{
	int ret;
	struct file *file;
	*copied_store = 0;
	if ((ret = fd2file(fd, &file)) != 0) {
		return ret;
	}
	if (!file->writable) {
		return -E_INVAL;
	}
	filemap_acquire(file);

	struct iobuf __iob, *iob = iobuf_init(&__iob, base, len, file->pos);
	ret = vop_write(file->node, iob);

	size_t copied = iobuf_used(iob);
	if (file->status == FD_OPENED) {
		file->pos += copied;
	}
	*copied_store = copied;
	filemap_release(file);
	return ret;
}

int file_seek(int fd, off_t pos, int whence)
{
	struct stat __stat, *stat = &__stat;
	int ret;
	struct file *file;
	if ((ret = fd2file(fd, &file)) != 0) {
		return ret;
	}
	filemap_acquire(file);

	switch (whence) {
	case LSEEK_SET:
		break;
	case LSEEK_CUR:
		pos += file->pos;
		break;
	case LSEEK_END:
		if ((ret = vop_fstat(file->node, stat)) == 0) {
			pos += stat->st_size;
		}
		break;
	default:
		ret = -E_INVAL;
	}

	if (ret == 0) {
		if ((ret = vop_tryseek(file->node, pos)) == 0) {
			file->pos = pos;
		}
	}
	filemap_release(file);
	return ret;
}

int file_fstat(int fd, struct stat *stat)
{
	int ret;
	struct file *file;
	if ((ret = fd2file(fd, &file)) != 0) {
		return ret;
	}
	filemap_acquire(file);
	ret = vop_fstat(file->node, stat);
	filemap_release(file);
	return ret;
}

int file_fsync(int fd)
{
	int ret;
	struct file *file;
	if ((ret = fd2file(fd, &file)) != 0) {
		return ret;
	}
	filemap_acquire(file);
	ret = vop_fsync(file->node);
	filemap_release(file);
	return ret;
}

int file_getdirentry(int fd, struct dirent *direntp)
{
	int ret;
	struct file *file;
	if ((ret = fd2file(fd, &file)) != 0) {
		return ret;
	}
	filemap_acquire(file);

	struct iobuf __iob, *iob =
	    iobuf_init(&__iob, direntp->d_name, sizeof(direntp->d_name),
		       direntp->d_off);
	if ((ret = vop_getdirentry(file->node, iob)) == 0) {
		direntp->d_off += iobuf_used(iob);
	}
	filemap_release(file);
	return ret;
}

int file_dup(int fd1, int fd2)
{
	int ret;
	struct file *file1, *file2;
	if ((ret = fd2file(fd1, &file1)) != 0) {
		return ret;
	}
	if ((ret = filemap_alloc(fd2, &file2)) != 0) {
		return ret;
	}
	filemap_dup(file2, file1);
	return file2->fd;
}

int file_pipe(int fd[])
{
	int ret;
	struct file *file[2] = { NULL, NULL };
	if ((ret = filemap_alloc(NO_FD, &file[0])) != 0) {
		goto failed_cleanup;
	}
	if ((ret = filemap_alloc(NO_FD, &file[1])) != 0) {
		goto failed_cleanup;
	}

	if ((ret = pipe_open(&(file[0]->node), &(file[1]->node))) != 0) {
		goto failed_cleanup;
	}
	file[0]->pos = 0;
	file[0]->readable = 1, file[0]->writable = 0;
	filemap_open(file[0]);

	file[1]->pos = 0;
	file[1]->readable = 0, file[1]->writable = 1;
	filemap_open(file[1]);

	fd[0] = file[0]->fd, fd[1] = file[1]->fd;
	return 0;

failed_cleanup:
	if (file[0] != NULL) {
		filemap_free(file[0]);
	}
	if (file[1] != NULL) {
		filemap_free(file[1]);
	}
	return ret;
}

int file_mkfifo(const char *__name, uint32_t open_flags)
{
	bool readonly = 0;
	switch (open_flags & O_ACCMODE) {
	case O_RDONLY:
		readonly = 1;
	case O_WRONLY:
		break;
	default:
		return -E_INVAL;
	}

	int ret;
	struct file *file;
	if ((ret = filemap_alloc(NO_FD, &file)) != 0) {
		return ret;
	}

	char *name;
	const char *device = readonly ? "pipe:r_" : "pipe:w_";

	if ((name = stradd(device, __name)) == NULL) {
		ret = -E_NO_MEM;
		goto failed_cleanup_file;
	}

	if ((ret = vfs_open(name, open_flags, &(file->node))) != 0) {
		goto failed_cleanup_name;
	}
	file->pos = 0;
	file->readable = readonly, file->writable = !readonly;
	filemap_open(file);
	kfree(name);
	return file->fd;

failed_cleanup_name:
	kfree(name);
failed_cleanup_file:
	filemap_free(file);
	return ret;
}

/* linux devfile adaptor */
bool __is_linux_devfile(int fd)
{
	int ret = -E_INVAL;
	struct file *file;
	if ((ret = fd2file(fd, &file)) != 0) {
		return 0;
	}
	if (file->node && check_inode_type(file->node, device)
	    && dev_is_linux_dev(vop_info(file->node, device)))
		return 1;
	return 0;
}

int linux_devfile_read(int fd, void *base, size_t len, size_t * copied_store)
{
	int ret = -E_INVAL;
	struct file *file;
	/* use 8byte int, in case of 64bit off_t
	 * config in linux kernel */
	int64_t offset;
	if ((ret = fd2file(fd, &file)) != 0) {
		return 0;
	}

	if (!file->readable) {
		return -E_INVAL;
	}
	filemap_acquire(file);
	offset = file->pos;
	struct device *dev = vop_info(file->node, device);
	assert(dev);
	ret = dev->d_linux_read(dev, base, len, (size_t *) & offset);
	if (ret >= 0) {
		*copied_store = (size_t) ret;
		file->pos += ret;
		ret = 0;
	}
	filemap_release(file);
	return ret;
}

int linux_devfile_write(int fd, void *base, size_t len, size_t * copied_store)
{
	int ret = -E_INVAL;
	struct file *file;
	/* use 8byte int, in case of 64bit off_t
	 * config in linux kernel */
	int64_t offset;
	if ((ret = fd2file(fd, &file)) != 0) {
		return 0;
	}

	if (!file->writable) {
		return -E_INVAL;
	}
	filemap_acquire(file);
	offset = file->pos;
	struct device *dev = vop_info(file->node, device);
	assert(dev);
	ret = dev->d_linux_write(dev, base, len, (size_t *) & offset);
	if (ret >= 0) {
		*copied_store = (size_t) ret;
		file->pos += ret;
		ret = 0;
	}
	filemap_release(file);
	return ret;
}

int linux_devfile_ioctl(int fd, unsigned int cmd, unsigned long arg)
{
	int ret = -E_INVAL;
	struct file *file;
	if ((ret = fd2file(fd, &file)) != 0) {
		return 0;
	}
	filemap_acquire(file);
	struct device *dev = vop_info(file->node, device);
	assert(dev);
	ret = dev->d_linux_ioctl(dev, cmd, arg);
	filemap_release(file);
	return ret;
}

void *linux_devfile_mmap2(void *addr, size_t len, int prot, int flags, int fd,
			  size_t pgoff)
{
	int ret = -E_INVAL;
	struct file *file;
	if ((ret = fd2file(fd, &file)) != 0) {
		return NULL;
	}
	filemap_acquire(file);
	struct device *dev = vop_info(file->node, device);
	assert(dev);
	void *r = dev->d_linux_mmap(dev, addr, len, prot, flags, pgoff);
	filemap_release(file);
	return r;
}

#ifdef UCONFIG_BIONIC_LIBC

int linux_access(char *path, int amode)
{
	/* do nothing but return 0 */
	return 0;
}

void *linux_regfile_mmap2(void *addr, size_t len, int prot, int flags, int fd,
			  size_t off)
{
	int subret = -E_INVAL;
	struct mm_struct *mm = current->mm;
	assert(mm != NULL);
	if (len == 0) {
		return -1;
	}
	lock_mm(mm);

	uintptr_t start = ROUNDDOWN(addr, PGSIZE);
	len = ROUNDUP(len, PGSIZE);

	uint32_t vm_flags = VM_READ;
	if (prot & PROT_WRITE) {
		vm_flags |= VM_WRITE;
	}
	if (prot & PROT_EXEC) {
		vm_flags |= VM_EXEC;
	}
	if (flags & MAP_STACK) {
		vm_flags |= VM_STACK;
	}
	if (flags & MAP_ANONYMOUS) {
		vm_flags |= VM_ANONYMOUS;
	}

	subret = -E_NO_MEM;
	if (start == 0 && (start = get_unmapped_area(mm, len)) == 0) {
		goto out_unlock;
	}
	uintptr_t end = start + len;
	struct vma_struct *vma = find_vma(mm, start);
	if (vma == NULL || vma->vm_start >= end) {
		vma = NULL;
	} else if (!(flags & MAP_FIXED)) {
		start = get_unmapped_area(mm, len);
		vma = NULL;
	} else if (!(vma->vm_flags & VM_ANONYMOUS)) {
		goto out_unlock;
	} else if (vma->vm_start == start && end == vma->vm_end) {
		vma->vm_flags = vm_flags;
	} else {
		assert(vma->vm_start <= start && end <= vma->vm_end);
		if ((subret = mm_unmap_keep_pages(mm, start, len)) != 0) {
			goto out_unlock;
		}
		vma = NULL;
	}
	if (vma == NULL
	    && (subret = mm_map(mm, start, len, vm_flags, &vma)) != 0) {
		goto out_unlock;
	}
	if (!(flags & MAP_ANONYMOUS)) {
		vma_mapfile(vma, fd, off << 12, NULL);
	}
	subret = 0;
out_unlock:
	unlock_mm(mm);
	return subret == 0 ? start : -1;
}

int filestruct_setpos(struct file *file, off_t pos)
{
	int ret = vop_tryseek(file->node, pos);
	if (ret == 0) {
		file->pos = pos;
	}
	return ret;
}

int filestruct_read(struct file *file, void *base, size_t len)
{
	struct iobuf __iob, *iob = iobuf_init(&__iob, base, len, file->pos);
	vop_read(file->node, iob);
	size_t copied = iobuf_used(iob);
	if (file->status == FD_OPENED) {
		file->pos += copied;
	}
	return copied;
}

#endif //UCONFIG_BIONIC_LIBC
