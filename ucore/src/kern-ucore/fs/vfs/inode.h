#ifndef __KERN_FS_VFS_INODE_H__
#define __KERN_FS_VFS_INODE_H__

#include <types.h>
#include <dev.h>
#include <pipe.h>
#include <sfs.h>
#include <fatfs/ffs.h>
#include <yaffs2_direct/yaffs_vfs.h>
#include <atomic.h>
#include <assert.h>

struct stat;
struct iobuf;

/*
 * A struct inode is an abstract representation of a file.
 *
 * It is an interface that allows the kernel's filesystem-independent 
 * code to interact usefully with multiple sets of filesystem code.
 */

/*
 * Abstract low-level file.
 *
 * Note: in_info is Filesystem-specific data, in_type is the inode type
 *
 * open_count is managed using VOP_INCOPEN and VOP_DECOPEN by
 * vfs_open() and vfs_close(). Code above the VFS layer should not
 * need to worry about it.
 */
struct inode {
	union {
		struct device __device_info;
		struct pipe_root __pipe_root_info;
		struct pipe_inode __pipe_inode_info;
		struct sfs_inode __sfs_inode_info;
#ifdef UCONFIG_HAVE_YAFFS2
		struct yaffs2_inode __yaffs2_inode_info;
#endif
#ifdef UCONFIG_HAVE_FATFS
		struct ffs_inode __ffs_inode_info;
#endif
	} in_info;
	enum {
		inode_type_device_info = 0x1234,
		inode_type_pipe_root_info,
		inode_type_pipe_inode_info,
		inode_type_sfs_inode_info,
#ifdef UCONFIG_HAVE_YAFFS2
		inode_type_yaffs2_inode_info,
#endif
#ifdef UCONFIG_HAVE_FATFS
		inode_type_ffs_inode_info,
#endif
	} in_type;
	atomic_t ref_count;
	atomic_t open_count;
	struct fs *in_fs;
	const struct inode_ops *in_ops;
#ifdef UCONFIG_BIONIC_LIBC
	list_entry_t mapped_addr_list;
#endif				//UCONFIG_BIONIC_LIBC
};

#define __in_type(type)                                             inode_type_##type##_info

#define check_inode_type(node, type)                                ((node)->in_type == __in_type(type))

#define __vop_info(node, type)                                      \
    ({                                                              \
        struct inode *__node = (node);                              \
        assert(__node != NULL && check_inode_type(__node, type));   \
        &(__node->in_info.__##type##_info);                         \
     })

#define vop_info(node, type)                                        __vop_info(node, type)

#define info2node(info, type)                                       \
    to_struct((info), struct inode, in_info.__##type##_info)

struct inode *__alloc_inode(int type);

#define alloc_inode(type)                                           __alloc_inode(__in_type(type))

#define MAX_INODE_COUNT                     0x10000

int inode_ref_inc(struct inode *node);
int inode_ref_dec(struct inode *node);
int inode_open_inc(struct inode *node);
int inode_open_dec(struct inode *node);

void inode_init(struct inode *node, const struct inode_ops *ops, struct fs *fs);
void inode_kill(struct inode *node);

#define VOP_MAGIC                           0x8c4ba476

/*
 * Abstract operations on a inode.
 *
 * These are used in the form VOP_FOO(inode, args), which are macros
 * that expands to inode->inode_ops->vop_foo(inode, args). The operations
 * "foo" are:
 *
 *    vop_open        - Called on open() of a file. Can be used to
 *                      reject illegal or undesired open modes. Note that
 *                      various operations can be performed without the
 *                      file actually being opened.
 *                      The inode need not look at O_CREAT, O_EXCL, or 
 *                      O_TRUNC, as these are handled in the VFS layer.
 *
 *                      VOP_EACHOPEN should not be called directly from
 *                      above the VFS layer - use vfs_open() to open inodes.
 *                      This maintains the open count so VOP_LASTCLOSE can
 *                      be called at the right time.
 *
 *    vop_close       - To be called on *last* close() of a file.
 *
 *                      VOP_LASTCLOSE should not be called directly from
 *                      above the VFS layer - use vfs_close() to close
 *                      inodes opened with vfs_open().
 *
 *    vop_reclaim     - Called when inode is no longer in use. Note that
 *                      this may be substantially after vop_lastclose is
 *                      called.
 *
 *****************************************
 *
 *    vop_read        - Read data from file to uio, at offset specified
 *                      in the uio, updating uio_resid to reflect the
 *                      amount read, and updating uio_offset to match.
 *                      Not allowed on directories or symlinks.
 *
 *    vop_readlink    - Read the contents of a symlink into a uio.
 *                      Not allowed on other types of object.
 *
 *    vop_getdirentry - Read a single filename from a directory into a
 *                      uio, choosing what name based on the offset
 *                      field in the uio, and updating that field.
 *                      Unlike with I/O on regular files, the value of
 *                      the offset field is not interpreted outside
 *                      the filesystem and thus need not be a byte
 *                      count. However, the uio_resid field should be
 *                      handled in the normal fashion.
 *                      On non-directory objects, return ENOTDIR.
 *
 *    vop_write       - Write data from uio to file at offset specified
 *                      in the uio, updating uio_resid to reflect the
 *                      amount written, and updating uio_offset to match.
 *                      Not allowed on directories or symlinks.
 *
 *    vop_ioctl       - Perform ioctl operation OP on file using data
 *                      DATA. The interpretation of the data is specific
 *                      to each ioctl.
 *
 *    vop_stat        - Return info about a file. The pointer is a 
 *                      pointer to struct stat; see kern/stat.h.
 *
 *    vop_gettype     - Return type of file. The values for file types
 *                      are in kern/stattypes.h.
 *
 *    vop_tryseek     - Check if seeking to the specified position within
 *                      the file is legal. (For instance, all seeks
 *                      are illegal on serial port devices, and seeks
 *                      past EOF on files whose sizes are fixed may be
 *                      as well.)
 *
 *    vop_fsync       - Force any dirty buffers associated with this file
 *                      to stable storage.
 *
 *    vop_truncate    - Forcibly set size of file to the length passed
 *                      in, discarding any excess blocks.
 *
 *    vop_namefile    - Compute pathname relative to filesystem root
 *                      of the file and copy to the specified
 *                      uio. Need not work on objects that are not
 *                      directories.
 *
 *****************************************
 *
 *    vop_creat       - Create a regular file named NAME in the passed
 *                      directory DIR. If boolean EXCL is true, fail if
 *                      the file already exists; otherwise, use the
 *                      existing file if there is one. Hand back the
 *                      inode for the file as per vop_lookup.
 *
 *    vop_symlink     - Create symlink named NAME in the passed directory,
 *                      with contents CONTENTS.
 *
 *    vop_mkdir       - Make directory NAME in the passed directory PARENTDIR.
 *
 *    vop_link        - Create hard link, with name NAME, to file FILE
 *                      in the passed directory DIR.
 *
 *    vop_unlink      - Delete non-directory object NAME from passed 
 *                      directory. If NAME refers to a directory,
 *                      return EISDIR. If passed inode is not a
 *                      directory, return ENOTDIR.
 *
 *    vop_rename      - Rename file NAME1 in directory VN1 to be
 *                      file NAME2 in directory VN2.
 *
 *****************************************
 *
 *    vop_lookup      - Parse PATHNAME relative to the passed directory
 *                      DIR, and hand back the inode for the file it
 *                      refers to. May destroy PATHNAME. Should increment
 *                      refcount on inode handed back.
 *
 *    vop_lookparent  - Parse PATHNAME relative to the passed directory
 *                      DIR, and hand back (1) the inode for the
 *                      parent directory of the file it refers to, and
 *                      (2) the last component of the filename, copied
 *                      into kernel buffer BUF with max length LEN. May
 *                      destroy PATHNAME. Should increment refcount on
 *                      inode handed back.
 */
struct inode_ops {
	unsigned long vop_magic;
	int (*vop_open) (struct inode * node, uint32_t open_flags);
	int (*vop_close) (struct inode * node);
	int (*vop_read) (struct inode * node, struct iobuf * iob);
	int (*vop_write) (struct inode * node, struct iobuf * iob);
	int (*vop_fstat) (struct inode * node, struct stat * stat);
	int (*vop_fsync) (struct inode * node);
	int (*vop_mkdir) (struct inode * node, const char *name);
	int (*vop_link) (struct inode * node, const char *name,
			 struct inode * link_node);
	int (*vop_rename) (struct inode * node, const char *name,
			   struct inode * new_node, const char *new_name);
	int (*vop_readlink) (struct inode * node, struct iobuf * iob);
	int (*vop_symlink) (struct inode * node, const char *name,
			    const char *path);
	int (*vop_namefile) (struct inode * node, struct iobuf * iob);
	int (*vop_getdirentry) (struct inode * node, struct iobuf * iob);
	int (*vop_reclaim) (struct inode * node);
	int (*vop_ioctl) (struct inode * node, int op, void *data);
	int (*vop_gettype) (struct inode * node, uint32_t * type_store);
	int (*vop_tryseek) (struct inode * node, off_t pos);
	int (*vop_truncate) (struct inode * node, off_t len);
	int (*vop_create) (struct inode * node, const char *name, bool excl,
			   struct inode ** node_store);
	int (*vop_unlink) (struct inode * node, const char *name);
	int (*vop_lookup) (struct inode * node, char *path,
			   struct inode ** node_store);
	int (*vop_lookup_parent) (struct inode * node, char *path,
				  struct inode ** node_store, char **endp);
};

int null_vop_pass(void);
int null_vop_inval(void);
int null_vop_unimp(void);
int null_vop_isdir(void);
int null_vop_notdir(void);

/*
 * Consistency check
 */
void inode_check(struct inode *node, const char *opstr);

#define __vop_op(node, sym)                                                                         \
    ({                                                                                              \
        struct inode *__node = (node);                                                              \
        assert(__node != NULL && __node->in_ops != NULL && __node->in_ops->vop_##sym != NULL);      \
        inode_check(__node, #sym);                                                                  \
        __node->in_ops->vop_##sym;                                                                  \
     })

#define vop_open(node, open_flags)                                  (__vop_op(node, open)(node, open_flags))
#define vop_close(node)                                             (__vop_op(node, close)(node))
#define vop_read(node, iob)                                         (__vop_op(node, read)(node, iob))
#define vop_write(node, iob)                                        (__vop_op(node, write)(node, iob))
#define vop_fstat(node, stat)                                       (__vop_op(node, fstat)(node, stat))
#define vop_fsync(node)                                             (__vop_op(node, fsync)(node))
#define vop_mkdir(node, name)                                       (__vop_op(node, mkdir)(node, name))
#define vop_link(node, name, link_node)                             (__vop_op(node, link)(node, name, link_node))
#define vop_rename(node, name, new_node, new_name)                  (__vop_op(node, rename)(node, name, new_node, new_name))
#define vop_readlink(node, iob)                                     (__vop_op(node, readlink)(node, iob))
#define vop_symlink(node, name, path)                               (__vop_op(node, symlink)(node, name, path))
#define vop_namefile(node, iob)                                     (__vop_op(node, namefile)(node, iob))
#define vop_getdirentry(node, iob)                                  (__vop_op(node, getdirentry)(node, iob))
#define vop_reclaim(node)                                           (__vop_op(node, reclaim)(node))
#define vop_ioctl(node, op, data)                                   (__vop_op(node, ioctl)(node, op, data))
#define vop_gettype(node, type_store)                               (__vop_op(node, gettype)(node, type_store))
#define vop_tryseek(node, pos)                                      (__vop_op(node, tryseek)(node, pos))
#define vop_truncate(node, len)                                     (__vop_op(node, truncate)(node, len))
#define vop_create(node, name, excl, node_store)                    (__vop_op(node, create)(node, name, excl, node_store))
#define vop_unlink(node, name)                                      (__vop_op(node, unlink)(node, name))
#define vop_lookup(node, path, node_store)                          (__vop_op(node, lookup)(node, path, node_store))
#define vop_lookup_parent(node, path, node_store, endp)             (__vop_op(node, lookup_parent)(node, path, node_store, endp))

#define vop_fs(node)                                                ((node)->in_fs)
#define vop_init(node, ops, fs)                                     inode_init(node, ops, fs)
#define vop_kill(node)                                              inode_kill(node)

/*
 * Reference count manipulation (handled above filesystem level)
 */
#define vop_ref_inc(node)                                           inode_ref_inc(node)
#define vop_ref_dec(node)                                           inode_ref_dec(node)
/*
 * Open count manipulation (handled above filesystem level)
 *
 * VOP_INCOPEN is called by vfs_open. VOP_DECOPEN is called by vfs_close.
 * Neither of these should need to be called from above the vfs layer.
 */
#define vop_open_inc(node)                                          inode_open_inc(node)
#define vop_open_dec(node)                                          inode_open_dec(node)
/* *
 * All these inode ops return int, and it's ok to cast function that take args
 * to functions that take no args.
 * Casting through void * prevents warnings.
 * */
#define NULL_VOP_PASS                                               ((void *)null_vop_pass)
#define NULL_VOP_INVAL                                              ((void *)null_vop_inval)
#define NULL_VOP_UNIMP                                              ((void *)null_vop_unimp)
#define NULL_VOP_ISDIR                                              ((void *)null_vop_isdir)
#define NULL_VOP_NOTDIR                                             ((void *)null_vop_notdir)

static inline int inode_ref_count(struct inode *node)
{
	return atomic_read(&(node->ref_count));
}

static inline int inode_open_count(struct inode *node)
{
	return atomic_read(&(node->open_count));
}

#endif /* !__KERN_FS_VFS_INODE_H__ */
