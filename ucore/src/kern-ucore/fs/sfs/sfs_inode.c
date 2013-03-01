#include <types.h>
#include <string.h>
#include <stdlib.h>
#include <slab.h>
#include <list.h>
#include <stat.h>
#include <vfs.h>
#include <dev.h>
#include <sfs.h>
#include <inode.h>
#include <iobuf.h>
#include <bitmap.h>
#include <error.h>
#include <assert.h>

static const struct inode_ops sfs_node_dirops;
static const struct inode_ops sfs_node_fileops;

static inline int trylock_sin(struct sfs_inode *sin)
{
	if (!SFSInodeRemoved(sin)) {
		down(&(sin->sem));
		if (!SFSInodeRemoved(sin)) {
			return 0;
		}
		up(&(sin->sem));
	}
	return -E_NOENT;
}

static inline void unlock_sin(struct sfs_inode *sin)
{
	up(&(sin->sem));
}

static const struct inode_ops *sfs_get_ops(uint16_t type)
{
	switch (type) {
	case SFS_TYPE_DIR:
		return &sfs_node_dirops;
	case SFS_TYPE_FILE:
		return &sfs_node_fileops;
	}
	panic("invalid file type %d.\n", type);
}

static list_entry_t *sfs_hash_list(struct sfs_fs *sfs, uint32_t ino)
{
	return sfs->hash_list + sin_hashfn(ino);
}

static void sfs_set_links(struct sfs_fs *sfs, struct sfs_inode *sin)
{
	list_add(&(sfs->inode_list), &(sin->inode_link));
	list_add(sfs_hash_list(sfs, sin->ino), &(sin->hash_link));
}

static void sfs_remove_links(struct sfs_inode *sin)
{
	list_del(&(sin->inode_link));
	list_del(&(sin->hash_link));
}

static bool sfs_block_inuse(struct sfs_fs *sfs, uint32_t ino)
{
	if (ino != 0 && ino < sfs->super.blocks) {
		return !bitmap_test(sfs->freemap, ino);
	}
	panic("sfs_block_inuse: called out of range (0, %u) %u.\n",
	      sfs->super.blocks, ino);
}

static int sfs_block_alloc(struct sfs_fs *sfs, uint32_t * ino_store)
{
	int ret;
	if ((ret = bitmap_alloc(sfs->freemap, ino_store)) != 0) {
		return ret;
	}
	assert(sfs->super.unused_blocks > 0);
	sfs->super.unused_blocks--, sfs->super_dirty = 1;
	assert(sfs_block_inuse(sfs, *ino_store));
	return sfs_clear_block(sfs, *ino_store, 1);
}

static void sfs_block_free(struct sfs_fs *sfs, uint32_t ino)
{
	assert(sfs_block_inuse(sfs, ino));
	bitmap_free(sfs->freemap, ino);
	sfs->super.unused_blocks++, sfs->super_dirty = 1;
}

static int
sfs_create_inode(struct sfs_fs *sfs, struct sfs_disk_inode *din, uint32_t ino,
		 struct inode **node_store)
{
	struct inode *node;
	if ((node = alloc_inode(sfs_inode)) != NULL) {
		vop_init(node, sfs_get_ops(din->type), info2fs(sfs, sfs));
		struct sfs_inode *sin = vop_info(node, sfs_inode);
		sin->din = din, sin->ino = ino, sin->dirty = 0, sin->flags =
		    0, sin->reclaim_count = 1;
		sem_init(&(sin->sem), 1);
		*node_store = node;
		return 0;
	}
	return -E_NO_MEM;
}

static struct inode *lookup_sfs_nolock(struct sfs_fs *sfs, uint32_t ino)
{
	struct inode *node;
	list_entry_t *list = sfs_hash_list(sfs, ino), *le = list;
	while ((le = list_next(le)) != list) {
		struct sfs_inode *sin = le2sin(le, hash_link);
		if (sin->ino == ino) {
			node = info2node(sin, sfs_inode);
			if (vop_ref_inc(node) == 1) {
				sin->reclaim_count++;
			}
			return node;
		}
	}
	return NULL;
}

int sfs_load_inode(struct sfs_fs *sfs, struct inode **node_store, uint32_t ino)
{
	lock_sfs_fs(sfs);
	struct inode *node;
	if ((node = lookup_sfs_nolock(sfs, ino)) != NULL) {
		goto out_unlock;
	}

	int ret = -E_NO_MEM;
	struct sfs_disk_inode *din;
	if ((din = kmalloc(sizeof(struct sfs_disk_inode))) == NULL) {
		goto failed_unlock;
	}

	assert(sfs_block_inuse(sfs, ino));
	if ((ret =
	     sfs_rbuf(sfs, din, sizeof(struct sfs_disk_inode), ino, 0)) != 0) {
		goto failed_cleanup_din;
	}

	assert(din->nlinks != 0);
	if ((ret = sfs_create_inode(sfs, din, ino, &node)) != 0) {
		goto failed_cleanup_din;
	}
	sfs_set_links(sfs, vop_info(node, sfs_inode));

out_unlock:
	unlock_sfs_fs(sfs);
	*node_store = node;
	return 0;

failed_cleanup_din:
	kfree(din);
failed_unlock:
	unlock_sfs_fs(sfs);
	return ret;
}

static int
sfs_bmap_get_sub_nolock(struct sfs_fs *sfs, uint32_t * entp, uint32_t index,
			bool create, uint32_t * ino_store)
{
	assert(index < SFS_BLK_NENTRY);
	int ret;
	uint32_t ent, ino = 0;
	off_t offset = index * sizeof(uint32_t);
	if ((ent = *entp) != 0) {
		if ((ret =
		     sfs_rbuf(sfs, &ino, sizeof(uint32_t), ent, offset)) != 0) {
			return ret;
		}
		if (ino != 0 || !create) {
			goto out;
		}
	} else {
		if (!create) {
			goto out;
		}
		if ((ret = sfs_block_alloc(sfs, &ent)) != 0) {
			return ret;
		}
	}

	if ((ret = sfs_block_alloc(sfs, &ino)) != 0) {
		goto failed_cleanup;
	}
	if ((ret = sfs_wbuf(sfs, &ino, sizeof(uint32_t), ent, offset)) != 0) {
		sfs_block_free(sfs, ino);
		goto failed_cleanup;
	}

out:
	if (ent != *entp) {
		*entp = ent;
	}
	*ino_store = ino;
	return 0;

failed_cleanup:
	if (ent != *entp) {
		sfs_block_free(sfs, ent);
	}
	return ret;
}

static int
sfs_bmap_get_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, uint32_t index,
		    bool create, uint32_t * ino_store)
{
	struct sfs_disk_inode *din = sin->din;
	int ret;
	uint32_t ent, ino;
	if (index < SFS_NDIRECT) {
		if ((ino = din->direct[index]) == 0 && create) {
			if ((ret = sfs_block_alloc(sfs, &ino)) != 0) {
				return ret;
			}
			din->direct[index] = ino;
			sin->dirty = 1;
		}
		goto out;
	}

	index -= SFS_NDIRECT;
	if (index < SFS_BLK_NENTRY) {
		ent = din->indirect;
		if ((ret =
		     sfs_bmap_get_sub_nolock(sfs, &ent, index, create,
					     &ino)) != 0) {
			return ret;
		}
		if (ent != din->indirect) {
			assert(din->indirect == 0);
			din->indirect = ent;
			sin->dirty = 1;
		}
		goto out;
	}

	index -= SFS_BLK_NENTRY;
	ent = din->db_indirect;
	if ((ret =
	     sfs_bmap_get_sub_nolock(sfs, &ent, index / SFS_BLK_NENTRY, create,
				     &ino)) != 0) {
		return ret;
	}
	if (ent != din->db_indirect) {
		assert(din->db_indirect == 0);
		din->db_indirect = ent;
		sin->dirty = 1;
	}
	if ((ent = ino) != 0) {
		if ((ret =
		     sfs_bmap_get_sub_nolock(sfs, &ent, index % SFS_BLK_NENTRY,
					     create, &ino)) != 0) {
			return ret;
		}
	}

out:
	assert(ino == 0 || sfs_block_inuse(sfs, ino));
	*ino_store = ino;
	return 0;
}

static int
sfs_bmap_free_sub_nolock(struct sfs_fs *sfs, uint32_t ent, uint32_t index)
{
	assert(sfs_block_inuse(sfs, ent) && index < SFS_BLK_NENTRY);
	int ret;
	uint32_t ino, zero = 0;
	off_t offset = index * sizeof(uint32_t);
	if ((ret = sfs_rbuf(sfs, &ino, sizeof(uint32_t), ent, offset)) != 0) {
		return ret;
	}
	if (ino != 0) {
		if ((ret =
		     sfs_wbuf(sfs, &zero, sizeof(uint32_t), ent,
			      offset)) != 0) {
			return ret;
		}
		sfs_block_free(sfs, ino);
	}
	return 0;
}

static int
sfs_bmap_free_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, uint32_t index)
{
	struct sfs_disk_inode *din = sin->din;
	int ret;
	uint32_t ent, ino;
	if (index < SFS_NDIRECT) {
		if ((ino = din->direct[index]) != 0) {
			sfs_block_free(sfs, ino);
			din->direct[index] = 0;
			sin->dirty = 1;
		}
		return 0;
	}

	index -= SFS_NDIRECT;
	if (index < SFS_BLK_NENTRY) {
		if ((ent = din->indirect) != 0) {
			if ((ret =
			     sfs_bmap_free_sub_nolock(sfs, ent, index)) != 0) {
				return ret;
			}
		}
		return 0;
	}

	index -= SFS_BLK_NENTRY;
	if ((ent = din->db_indirect) != 0) {
		if ((ret =
		     sfs_bmap_get_sub_nolock(sfs, &ent, index / SFS_BLK_NENTRY,
					     0, &ino)) != 0) {
			return ret;
		}
		if ((ent = ino) != 0) {
			if ((ret =
			     sfs_bmap_free_sub_nolock(sfs, ent,
						      index %
						      SFS_BLK_NENTRY)) != 0) {
				return ret;
			}
		}
	}
	return 0;
}

static int
sfs_bmap_load_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, uint32_t index,
		     uint32_t * ino_store)
{
	struct sfs_disk_inode *din = sin->din;
	assert(index <= din->blocks);
	int ret;
	uint32_t ino;
	bool create = (index == din->blocks);
	if ((ret = sfs_bmap_get_nolock(sfs, sin, index, create, &ino)) != 0) {
		return ret;
	}
	assert(sfs_block_inuse(sfs, ino));
	if (create) {
		din->blocks++;
	}
	if (ino_store != NULL) {
		*ino_store = ino;
	}
	return 0;
}

static int sfs_bmap_truncate_nolock(struct sfs_fs *sfs, struct sfs_inode *sin)
{
	struct sfs_disk_inode *din = sin->din;
	assert(din->blocks != 0);
	int ret;
	if ((ret = sfs_bmap_free_nolock(sfs, sin, din->blocks - 1)) != 0) {
		return ret;
	}
	din->blocks--;
	sin->dirty = 1;
	return 0;
}

static int
sfs_dirent_read_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, int slot,
		       struct sfs_disk_entry *entry)
{
	assert(sin->din->type == SFS_TYPE_DIR
	       && (slot >= 0 && slot < sin->din->blocks));
	int ret;
	uint32_t ino;
	if ((ret = sfs_bmap_load_nolock(sfs, sin, slot, &ino)) != 0) {
		return ret;
	}
	assert(sfs_block_inuse(sfs, ino));
	if ((ret =
	     sfs_rbuf(sfs, entry, sizeof(struct sfs_disk_entry), ino,
		      0)) != 0) {
		return ret;
	}
	entry->name[SFS_MAX_FNAME_LEN] = '\0';
	return 0;
}

static int
sfs_dirent_write_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, int slot,
			uint32_t ino, const char *name)
{
	assert(sin->din->type == SFS_TYPE_DIR
	       && (slot >= 0 && slot <= sin->din->blocks));
	struct sfs_disk_entry *entry;
	if ((entry = kmalloc(sizeof(struct sfs_disk_entry))) == NULL) {
		return -E_NO_MEM;
	}
	memset(entry, 0, sizeof(struct sfs_disk_entry));

	if (ino != 0) {
		assert(strlen(name) <= SFS_MAX_FNAME_LEN);
		entry->ino = ino, strcpy(entry->name, name);
	}
	int ret;
	if ((ret = sfs_bmap_load_nolock(sfs, sin, slot, &ino)) != 0) {
		goto out;
	}
	assert(sfs_block_inuse(sfs, ino));
	ret = sfs_wbuf(sfs, entry, sizeof(struct sfs_disk_entry), ino, 0);
out:
	kfree(entry);
	return ret;
}

static int
sfs_dirent_link_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, int slot,
		       struct sfs_inode *lnksin, const char *name)
{
	int ret;
	if ((ret =
	     sfs_dirent_write_nolock(sfs, sin, slot, lnksin->ino, name)) != 0) {
		return ret;
	}
	sin->dirty = 1;
	sin->din->dirinfo.slots++;
	lnksin->dirty = 1;
	lnksin->din->nlinks++;
	return 0;
}

static int
sfs_dirent_unlink_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, int slot,
			 struct sfs_inode *lnksin)
{
	int ret;
	if ((ret = sfs_dirent_write_nolock(sfs, sin, slot, 0, NULL)) != 0) {
		return ret;
	}
	assert(sin->din->dirinfo.slots != 0 && lnksin->din->nlinks != 0);
	sin->dirty = 1;
	sin->din->dirinfo.slots--;
	lnksin->dirty = 1;
	lnksin->din->nlinks--;
	return 0;
}

#define sfs_dirent_link_nolock_check(sfs, sin, slot, lnksin, name)                  \
    ({                                                                              \
        int err;                                                                    \
        if ((err = sfs_dirent_link_nolock(sfs, sin, slot, lnksin, name)) != 0) {    \
            warn("sfs_dirent_link error: %e.\n", err);                              \
        }                                                                           \
        err;                                                                        \
    })

#define sfs_dirent_unlink_nolock_check(sfs, sin, slot, lnksin)                      \
    ({                                                                              \
        int err;                                                                    \
        if ((err = sfs_dirent_unlink_nolock(sfs, sin, slot, lnksin)) != 0) {        \
            warn("sfs_dirent_unlink error: %e.\n", err);                            \
        }                                                                           \
        err;                                                                        \
    })

static int
sfs_dirent_search_nolock(struct sfs_fs *sfs, struct sfs_inode *sin,
			 const char *name, uint32_t * ino_store, int *slot,
			 int *empty_slot)
{
	assert(strlen(name) <= SFS_MAX_FNAME_LEN);
	struct sfs_disk_entry *entry;
	if ((entry = kmalloc(sizeof(struct sfs_disk_entry))) == NULL) {
		return -E_NO_MEM;
	}
#define set_pvalue(x, v)            do { if ((x) != NULL) { *(x) = (v); } } while (0)
	int ret, i, nslots = sin->din->blocks;
	set_pvalue(empty_slot, nslots);
	for (i = 0; i < nslots; i++) {
		if ((ret = sfs_dirent_read_nolock(sfs, sin, i, entry)) != 0) {
			goto out;
		}
		if (entry->ino == 0) {
			set_pvalue(empty_slot, i);
			continue;
		}
		if (strcmp(name, entry->name) == 0) {
			set_pvalue(slot, i);
			set_pvalue(ino_store, entry->ino);
			goto out;
		}
	}
#undef set_pvalue
	ret = -E_NOENT;
out:
	kfree(entry);
	return ret;
}

static int
sfs_dirent_findino_nolock(struct sfs_fs *sfs, struct sfs_inode *sin,
			  uint32_t ino, struct sfs_disk_entry *entry)
{
	int ret, i, nslots = sin->din->blocks;
	for (i = 0; i < nslots; i++) {
		if ((ret = sfs_dirent_read_nolock(sfs, sin, i, entry)) != 0) {
			return ret;
		}
		if (entry->ino == ino) {
			return 0;
		}
	}
	return -E_NOENT;
}

static int
sfs_dirent_create_inode(struct sfs_fs *sfs, uint16_t type,
			struct inode **node_store)
{
	struct sfs_disk_inode *din;
	if ((din = kmalloc(sizeof(struct sfs_disk_inode))) == NULL) {
		return -E_NO_MEM;
	}
	memset(din, 0, sizeof(struct sfs_disk_inode));
	din->type = type;

	int ret;
	uint32_t ino;
	if ((ret = sfs_block_alloc(sfs, &ino)) != 0) {
		goto failed_cleanup_din;
	}
	struct inode *node;
	if ((ret = sfs_create_inode(sfs, din, ino, &node)) != 0) {
		goto failed_cleanup_ino;
	}
	lock_sfs_fs(sfs);
	{
		sfs_set_links(sfs, vop_info(node, sfs_inode));
	}
	unlock_sfs_fs(sfs);
	*node_store = node;
	return 0;

failed_cleanup_ino:
	sfs_block_free(sfs, ino);
failed_cleanup_din:
	kfree(din);
	return ret;
}

static int
sfs_load_parent(struct sfs_fs *sfs, struct sfs_inode *sin,
		struct inode **parent_store)
{
	return sfs_load_inode(sfs, parent_store, sin->din->dirinfo.parent);
}

static int
sfs_lookup_once(struct sfs_fs *sfs, struct sfs_inode *sin, const char *name,
		struct inode **node_store, int *slot)
{
	int ret;
	if ((ret = trylock_sin(sin)) != 0) {
		return ret;
	}
	uint32_t ino;
	ret = sfs_dirent_search_nolock(sfs, sin, name, &ino, slot, NULL);
	unlock_sin(sin);
	if (ret != 0) {
		return ret;
	}
	return sfs_load_inode(sfs, node_store, ino);
}

static int sfs_opendir(struct inode *node, uint32_t open_flags)
{
	switch (open_flags & O_ACCMODE) {
	case O_RDONLY:
		break;
	case O_WRONLY:
	case O_RDWR:
	default:
		return -E_ISDIR;
	}
	if (open_flags & O_APPEND) {
		return -E_ISDIR;
	}
	return 0;
}

static int sfs_openfile(struct inode *node, uint32_t open_flags)
{
	return 0;
}

static int sfs_close(struct inode *node)
{
	return vop_fsync(node);
}

static int
sfs_io_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, void *buf,
	      off_t offset, size_t * alenp, bool write)
{
	struct sfs_disk_inode *din = sin->din;
	assert(din->type != SFS_TYPE_DIR);
	off_t endpos = offset + *alenp, blkoff;
	*alenp = 0;
	if (offset < 0 || offset >= SFS_MAX_FILE_SIZE || offset > endpos) {
		return -E_INVAL;
	}
	if (offset == endpos) {
		return 0;
	}
	if (endpos > SFS_MAX_FILE_SIZE) {
		endpos = SFS_MAX_FILE_SIZE;
	}
	if (!write) {
		if (offset >= din->fileinfo.size) {
			return 0;
		}
		if (endpos > din->fileinfo.size) {
			endpos = din->fileinfo.size;
		}
	}

	int (*sfs_buf_op) (struct sfs_fs * sfs, void *buf, size_t len,
			   uint32_t blkno, off_t offset);
	int (*sfs_block_op) (struct sfs_fs * sfs, void *buf, uint32_t blkno,
			     uint32_t nblks);
	if (write) {
		sfs_buf_op = sfs_wbuf, sfs_block_op = sfs_wblock;
	} else {
		sfs_buf_op = sfs_rbuf, sfs_block_op = sfs_rblock;
	}

	int ret = 0;
	size_t size, alen = 0;
	uint32_t ino;
	uint32_t blkno = offset / SFS_BLKSIZE;
	uint32_t nblks = endpos / SFS_BLKSIZE - blkno;

	if ((blkoff = offset % SFS_BLKSIZE) != 0) {
		size =
		    (nblks != 0) ? (SFS_BLKSIZE - blkoff) : (endpos - offset);
		if ((ret = sfs_bmap_load_nolock(sfs, sin, blkno, &ino)) != 0) {
			goto out;
		}
		if ((ret = sfs_buf_op(sfs, buf, size, ino, blkoff)) != 0) {
			goto out;
		}
		alen += size;
		if (nblks == 0) {
			goto out;
		}
		buf += size, blkno++, nblks--;
	}

	size = SFS_BLKSIZE;
	while (nblks != 0) {
		if ((ret = sfs_bmap_load_nolock(sfs, sin, blkno, &ino)) != 0) {
			goto out;
		}
		if ((ret = sfs_block_op(sfs, buf, ino, 1)) != 0) {
			goto out;
		}
		alen += size, buf += size, blkno++, nblks--;
	}

	if ((size = endpos % SFS_BLKSIZE) != 0) {
		if ((ret = sfs_bmap_load_nolock(sfs, sin, blkno, &ino)) != 0) {
			goto out;
		}
		if ((ret = sfs_buf_op(sfs, buf, size, ino, 0)) != 0) {
			goto out;
		}
		alen += size;
	}

out:
	*alenp = alen;
	if (offset + alen > din->fileinfo.size) {
		din->fileinfo.size = offset + alen;
		sin->dirty = 1;
	}
	return ret;
}

static inline int sfs_io(struct inode *node, struct iobuf *iob, bool write)
{
	struct sfs_fs *sfs = fsop_info(vop_fs(node), sfs);
	struct sfs_inode *sin = vop_info(node, sfs_inode);
	int ret;
	if ((ret = trylock_sin(sin)) != 0) {
		return ret;
	}
	size_t alen = iob->io_resid;
	ret =
	    sfs_io_nolock(sfs, sin, iob->io_base, iob->io_offset, &alen, write);
	if (alen != 0) {
		iobuf_skip(iob, alen);
	}
	unlock_sin(sin);
	return ret;
}

static int sfs_read(struct inode *node, struct iobuf *iob)
{
	return sfs_io(node, iob, 0);
}

static int sfs_write(struct inode *node, struct iobuf *iob)
{
	return sfs_io(node, iob, 1);
}

static int sfs_fstat(struct inode *node, struct stat *stat)
{
	int ret;
	memset(stat, 0, sizeof(struct stat));
	if ((ret = vop_gettype(node, &(stat->st_mode))) != 0) {
		return ret;
	}
	struct sfs_disk_inode *din = vop_info(node, sfs_inode)->din;
	stat->st_nlinks = din->nlinks;
	stat->st_blocks = din->blocks;
	if (din->type != SFS_TYPE_DIR) {
		stat->st_size = din->fileinfo.size;
	} else {
		stat->st_size = (din->dirinfo.slots + 2) * sfs_dentry_size;
	}
	return 0;
}

static inline void sfs_nlinks_inc_nolock(struct sfs_inode *sin)
{
	sin->dirty = 1, ++sin->din->nlinks;
}

static inline void sfs_nlinks_dec_nolock(struct sfs_inode *sin)
{
	assert(sin->din->nlinks != 0);
	sin->dirty = 1, --sin->din->nlinks;
}

static inline void
sfs_dirinfo_set_parent(struct sfs_inode *sin, struct sfs_inode *parent)
{
	sin->dirty = 1;
	sin->din->dirinfo.parent = parent->ino;
}

static int sfs_fsync(struct inode *node)
{
	struct sfs_fs *sfs = fsop_info(vop_fs(node), sfs);
	struct sfs_inode *sin = vop_info(node, sfs_inode);
	if (sin->din->nlinks == 0 || !sin->dirty) {
		return 0;
	}
	int ret;
	if ((ret = trylock_sin(sin)) != 0) {
		return ret;
	}
	if (sin->dirty) {
		sin->dirty = 0;
		if ((ret =
		     sfs_wbuf(sfs, sin->din, sizeof(struct sfs_disk_inode),
			      sin->ino, 0)) != 0) {
			sin->dirty = 1;
		}
	}
	unlock_sin(sin);
	return ret;
}

static int
sfs_mkdir_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, const char *name)
{
	int ret, slot;
	if ((ret =
	     sfs_dirent_search_nolock(sfs, sin, name, NULL, NULL,
				      &slot)) != -E_NOENT) {
		return (ret != 0) ? ret : -E_EXISTS;
	}
	struct inode *link_node;
	if ((ret = sfs_dirent_create_inode(sfs, SFS_TYPE_DIR, &link_node)) != 0) {
		return ret;
	}
	struct sfs_inode *lnksin = vop_info(link_node, sfs_inode);
	if ((ret = sfs_dirent_link_nolock(sfs, sin, slot, lnksin, name)) != 0) {
		assert(lnksin->din->nlinks == 0);
		assert(inode_ref_count(link_node) == 1
		       && inode_open_count(link_node) == 0);
		goto out;
	}

	/* set parent */
	sfs_dirinfo_set_parent(lnksin, sin);

	/* add '.' link to itself */
	sfs_nlinks_inc_nolock(lnksin);

	/* add '..' link to parent */
	sfs_nlinks_inc_nolock(sin);

out:
	vop_ref_dec(link_node);
	return ret;
}

static int sfs_mkdir(struct inode *node, const char *name)
{
	if (strlen(name) > SFS_MAX_FNAME_LEN) {
		return -E_TOO_BIG;
	}
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		return -E_EXISTS;
	}
	struct sfs_fs *sfs = fsop_info(vop_fs(node), sfs);
	struct sfs_inode *sin = vop_info(node, sfs_inode);
	int ret;
	if ((ret = trylock_sin(sin)) == 0) {
		ret = sfs_mkdir_nolock(sfs, sin, name);
		unlock_sin(sin);
	}
	return ret;
}

static int
sfs_link_nolock(struct sfs_fs *sfs, struct sfs_inode *sin,
		struct sfs_inode *lnksin, const char *name)
{
	int ret, slot;
	if ((ret =
	     sfs_dirent_search_nolock(sfs, sin, name, NULL, NULL,
				      &slot)) != -E_NOENT) {
		return (ret != 0) ? ret : -E_EXISTS;
	}
	return sfs_dirent_link_nolock(sfs, sin, slot, lnksin, name);
}

static int
sfs_link(struct inode *node, const char *name, struct inode *link_node)
{
	if (strlen(name) > SFS_MAX_FNAME_LEN) {
		return -E_TOO_BIG;
	}
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		return -E_EXISTS;
	}
	struct sfs_inode *lnksin = vop_info(link_node, sfs_inode);
	if (lnksin->din->type == SFS_TYPE_DIR) {
		return -E_ISDIR;
	}
	struct sfs_fs *sfs = fsop_info(vop_fs(node), sfs);
	struct sfs_inode *sin = vop_info(node, sfs_inode);
	int ret;
	if ((ret = trylock_sin(sin)) == 0) {
		ret = sfs_link_nolock(sfs, sin, lnksin, name);
		unlock_sin(sin);
	}
	return ret;
}

static int
sfs_rename1_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, const char *name,
		   const char *new_name)
{
	if (strcmp(name, new_name) == 0) {
		return 0;
	}
	int ret, slot;
	uint32_t ino;
	if ((ret =
	     sfs_dirent_search_nolock(sfs, sin, name, &ino, &slot,
				      NULL)) != 0) {
		return ret;
	}
	if ((ret =
	     sfs_dirent_search_nolock(sfs, sin, new_name, NULL, NULL,
				      NULL)) != -E_NOENT) {
		return (ret != 0) ? ret : -E_EXISTS;
	}
	return sfs_dirent_write_nolock(sfs, sin, slot, ino, new_name);
}

static int
sfs_rename2_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, const char *name,
		   struct sfs_inode *newsin, const char *new_name)
{
	uint32_t ino;
	int ret, slot1, slot2;
	if ((ret =
	     sfs_dirent_search_nolock(sfs, sin, name, &ino, &slot1,
				      NULL)) != 0) {
		return ret;
	}
	if ((ret =
	     sfs_dirent_search_nolock(sfs, newsin, new_name, NULL, NULL,
				      &slot2)) != -E_NOENT) {
		return (ret != 0) ? ret : -E_EXISTS;
	}

	struct inode *link_node;
	if ((ret = sfs_load_inode(sfs, &link_node, ino)) != 0) {
		return ret;
	}

	struct sfs_inode *lnksin = vop_info(link_node, sfs_inode);
	if ((ret = sfs_dirent_unlink_nolock(sfs, sin, slot1, lnksin)) != 0) {
		goto out;
	}

	int isdir = (lnksin->din->type == SFS_TYPE_DIR);

	/* remove '..' link from old parent */
	if (isdir) {
		sfs_nlinks_dec_nolock(sin);
	}

	/* if link fails try to recover its old link */
	if ((ret =
	     sfs_dirent_link_nolock(sfs, newsin, slot2, lnksin,
				    new_name)) != 0) {
		if (sfs_dirent_link_nolock_check(sfs, sin, slot1, lnksin, name)
		    == 0) {
			if (isdir) {
				sfs_nlinks_inc_nolock(sin);
			}
		}
		goto out;
	}

	if (isdir) {
		/* set '..' link to new directory */
		sfs_nlinks_inc_nolock(newsin);

		/* update parent relationship */
		sfs_dirinfo_set_parent(lnksin, newsin);
	}

out:
	vop_ref_dec(link_node);
	return ret;
}

static int
sfs_rename(struct inode *node, const char *name, struct inode *new_node,
	   const char *new_name)
{
	if (strlen(name) > SFS_MAX_FNAME_LEN
	    || strlen(new_name) > SFS_MAX_FNAME_LEN) {
		return -E_TOO_BIG;
	}
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		return -E_INVAL;
	}
	if (strcmp(new_name, ".") == 0 || strcmp(new_name, "..") == 0) {
		return -E_EXISTS;
	}
	struct sfs_fs *sfs = fsop_info(vop_fs(node), sfs);
	struct sfs_inode *sin = vop_info(node, sfs_inode), *newsin =
	    vop_info(new_node, sfs_inode);
	int ret;
	lock_sfs_mutex(sfs);
	{
		if ((ret = trylock_sin(sin)) == 0) {
			if (sin == newsin) {
				ret =
				    sfs_rename1_nolock(sfs, sin, name,
						       new_name);
			} else if ((ret = trylock_sin(newsin)) == 0) {
				ret =
				    sfs_rename2_nolock(sfs, sin, name, newsin,
						       new_name);
				unlock_sin(newsin);
			}
			unlock_sin(sin);
		}
	}
	unlock_sfs_mutex(sfs);
	return ret;
}

static int sfs_namefile(struct inode *node, struct iobuf *iob)
{
	struct sfs_disk_entry *entry;
	if (iob->io_resid <= 2
	    || (entry = kmalloc(sizeof(struct sfs_disk_entry))) == NULL) {
		return -E_NO_MEM;
	}

	struct sfs_fs *sfs = fsop_info(vop_fs(node), sfs);
	struct sfs_inode *sin = vop_info(node, sfs_inode);

	int ret;
	uint32_t ino;
	char *ptr = iob->io_base + iob->io_resid;
	size_t alen, resid = iob->io_resid - 2;

	vop_ref_inc(node);
	while ((ino = sin->ino) != SFS_BLKN_ROOT) {
		struct inode *parent;
		if ((ret = sfs_load_parent(sfs, sin, &parent)) != 0) {
			goto failed;
		}
		vop_ref_dec(node);

		node = parent, sin = vop_info(node, sfs_inode);
		assert(ino != sin->ino && sin->din->type == SFS_TYPE_DIR);

		if ((ret = trylock_sin(sin)) != 0) {
			goto failed;
		}
		ret = sfs_dirent_findino_nolock(sfs, sin, ino, entry);
		unlock_sin(sin);

		if (ret != 0) {
			goto failed;
		}

		if ((alen = strlen(entry->name) + 1) > resid) {
			goto failed_nomem;
		}
		resid -= alen, ptr -= alen;
		memcpy(ptr, entry->name, alen - 1);
		ptr[alen - 1] = '/';
	}
	vop_ref_dec(node);
	alen = iob->io_resid - resid - 2;
	ptr = memmove(iob->io_base + 1, ptr, alen);
	ptr[-1] = '/', ptr[alen] = '\0';
	iobuf_skip(iob, alen);
	kfree(entry);
	return 0;

failed_nomem:
	ret = -E_NO_MEM;
failed:
	vop_ref_dec(node);
	kfree(entry);
	return ret;
}

static int
sfs_getdirentry_sub_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, int slot,
			   struct sfs_disk_entry *entry)
{
	int ret, i, nslots = sin->din->blocks;
	for (i = 0; i < nslots; i++) {
		if ((ret = sfs_dirent_read_nolock(sfs, sin, i, entry)) != 0) {
			return ret;
		}
		if (entry->ino != 0) {
			if (slot == 0) {
				return 0;
			}
			slot--;
		}
	}
	return -E_NOENT;
}

static int sfs_getdirentry(struct inode *node, struct iobuf *iob)
{
	struct sfs_disk_entry *entry;
	if ((entry = kmalloc(sizeof(struct sfs_disk_entry))) == NULL) {
		return -E_NO_MEM;
	}

	struct sfs_fs *sfs = fsop_info(vop_fs(node), sfs);
	struct sfs_inode *sin = vop_info(node, sfs_inode);

	off_t offset = iob->io_offset;
	if (offset < 0 || offset % sfs_dentry_size != 0) {
		kfree(entry);
		return -E_INVAL;
	}

	int ret, slot = offset / sfs_dentry_size;
	if (slot >= sin->din->dirinfo.slots + 2) {
		kfree(entry);
		return -E_NOENT;
	}
	switch (slot) {
	case 0:
		strcpy(entry->name, ".");
		break;
	case 1:
		strcpy(entry->name, "..");
		break;
	default:
		if ((ret = trylock_sin(sin)) != 0) {
			goto out;
		}
		ret = sfs_getdirentry_sub_nolock(sfs, sin, slot - 2, entry);
		unlock_sin(sin);
		if (ret != 0) {
			goto out;
		}
	}
	ret = iobuf_move(iob, entry->name, sfs_dentry_size, 1, NULL);
out:
	kfree(entry);
	return ret;
}

static int sfs_reclaim(struct inode *node)
{
	struct sfs_fs *sfs = fsop_info(vop_fs(node), sfs);
	struct sfs_inode *sin = vop_info(node, sfs_inode);

	lock_sfs_fs(sfs);

	int ret = -E_BUSY;
	assert(sin->reclaim_count > 0);
	if ((--sin->reclaim_count) != 0) {
		goto failed_unlock;
	}
	assert(inode_ref_count(node) == 0 && inode_open_count(node) == 0);

	if (sin->din->nlinks == 0) {
		uint32_t nblks;
		for (nblks = sin->din->blocks; nblks != 0; nblks--) {
			sfs_bmap_truncate_nolock(sfs, sin);
		}
	} else if (sin->dirty) {
		if ((ret = vop_fsync(node)) != 0) {
			goto failed_unlock;
		}
	}

	sfs_remove_links(sin);
	unlock_sfs_fs(sfs);

	if (sin->din->nlinks == 0) {
		sfs_block_free(sfs, sin->ino);
		uint32_t ent;
		if ((ent = sin->din->indirect) != 0) {
			sfs_block_free(sfs, ent);
		}
		if ((ent = sin->din->db_indirect) != 0) {
			int i;
			for (i = 0; i < SFS_BLK_NENTRY; i++) {
				sfs_bmap_free_sub_nolock(sfs, ent, i);
			}
			sfs_block_free(sfs, ent);
		}
	}
	kfree(sin->din);
	vop_kill(node);
	return 0;

failed_unlock:
	unlock_sfs_fs(sfs);
	return ret;
}

static int sfs_gettype(struct inode *node, uint32_t * type_store)
{
	struct sfs_disk_inode *din = vop_info(node, sfs_inode)->din;
	switch (din->type) {
	case SFS_TYPE_DIR:
		*type_store = S_IFDIR;
		return 0;
	case SFS_TYPE_FILE:
		*type_store = S_IFREG;
		return 0;
	case SFS_TYPE_LINK:
		*type_store = S_IFLNK;
		return 0;
	}
	panic("invalid file type %d.\n", din->type);
}

static int sfs_tryseek(struct inode *node, off_t pos)
{
	if (pos < 0 || pos >= SFS_MAX_FILE_SIZE) {
		return -E_INVAL;
	}
	struct sfs_disk_inode *din = vop_info(node, sfs_inode)->din;
	if (pos > din->fileinfo.size) {
		return vop_truncate(node, pos);
	}
	return 0;
}

static int sfs_truncfile(struct inode *node, off_t len)
{
	if (len < 0 || len > SFS_MAX_FILE_SIZE) {
		return -E_INVAL;
	}
	struct sfs_fs *sfs = fsop_info(vop_fs(node), sfs);
	struct sfs_inode *sin = vop_info(node, sfs_inode);
	struct sfs_disk_inode *din = sin->din;
	assert(din->type != SFS_TYPE_DIR);

	int ret = 0;
	uint32_t nblks, tblks = ROUNDUP_DIV(len, SFS_BLKSIZE);
	if (din->fileinfo.size == len) {
		assert(tblks == din->blocks);
		return 0;
	}

	if ((ret = trylock_sin(sin)) != 0) {
		return ret;
	}
	nblks = din->blocks;
	if (nblks < tblks) {
		while (nblks != tblks) {
			if ((ret =
			     sfs_bmap_load_nolock(sfs, sin, nblks,
						  NULL)) != 0) {
				goto out_unlock;
			}
			nblks++;
		}
	} else if (tblks < nblks) {
		while (tblks != nblks) {
			if ((ret = sfs_bmap_truncate_nolock(sfs, sin)) != 0) {
				goto out_unlock;
			}
			nblks--;
		}
	}
	assert(din->blocks == tblks);
	din->fileinfo.size = len;
	sin->dirty = 1;

out_unlock:
	unlock_sin(sin);
	return ret;
}

static int
sfs_create_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, const char *name,
		  bool excl, struct inode **node_store)
{
	int ret, slot;
	uint32_t ino;
	struct inode *link_node;
	if ((ret =
	     sfs_dirent_search_nolock(sfs, sin, name, &ino, NULL,
				      &slot)) != -E_NOENT) {
		if (ret != 0) {
			return ret;
		}
		if (!excl) {
			if ((ret = sfs_load_inode(sfs, &link_node, ino)) != 0) {
				return ret;
			}
			if (vop_info(link_node, sfs_inode)->din->type ==
			    SFS_TYPE_FILE) {
				goto out;
			}
			vop_ref_dec(link_node);
		}
		return -E_EXISTS;
	} else {
		if ((ret =
		     sfs_dirent_create_inode(sfs, SFS_TYPE_FILE,
					     &link_node)) != 0) {
			return ret;
		}
		if ((ret =
		     sfs_dirent_link_nolock(sfs, sin, slot,
					    vop_info(link_node, sfs_inode),
					    name)) != 0) {
			vop_ref_dec(link_node);
			return ret;
		}
	}

out:
	*node_store = link_node;
	return 0;
}

static int
sfs_create(struct inode *node, const char *name, bool excl,
	   struct inode **node_store)
{
	if (strlen(name) > SFS_MAX_FNAME_LEN) {
		return -E_TOO_BIG;
	}
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		return -E_EXISTS;
	}
	struct sfs_fs *sfs = fsop_info(vop_fs(node), sfs);
	struct sfs_inode *sin = vop_info(node, sfs_inode);
	int ret;
	if ((ret = trylock_sin(sin)) == 0) {
		ret = sfs_create_nolock(sfs, sin, name, excl, node_store);
		unlock_sin(sin);
	}
	return ret;
}

static int
sfs_unlink_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, const char *name)
{
	int ret, slot;
	uint32_t ino;
	if ((ret =
	     sfs_dirent_search_nolock(sfs, sin, name, &ino, &slot,
				      NULL)) != 0) {
		return ret;
	}
	struct inode *link_node;
	if ((ret = sfs_load_inode(sfs, &link_node, ino)) != 0) {
		return ret;
	}
	struct sfs_inode *lnksin = vop_info(link_node, sfs_inode);
	if (lnksin->din->type != SFS_TYPE_DIR) {
		ret = sfs_dirent_unlink_nolock(sfs, sin, slot, lnksin);
	} else {
		if ((ret = trylock_sin(lnksin)) == 0) {
			if (lnksin->din->dirinfo.slots != 0) {
				ret = -E_NOTEMPTY;
			} else
			    if ((ret =
				 sfs_dirent_unlink_nolock(sfs, sin, slot,
							  lnksin)) == 0) {
				/* lnksin must be empty, so set SFS_removed bit to invalidate further trylock opts */
				SetSFSInodeRemoved(lnksin);

				/* remove '.' link */
				sfs_nlinks_dec_nolock(lnksin);

				/* remove '..' link */
				sfs_nlinks_dec_nolock(sin);
			}
			unlock_sin(lnksin);
		}
	}
	vop_ref_dec(link_node);
	return ret;
}

static int sfs_unlink(struct inode *node, const char *name)
{
	if (strlen(name) > SFS_MAX_FNAME_LEN) {
		return -E_TOO_BIG;
	}
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
		return -E_ISDIR;
	}
	struct sfs_fs *sfs = fsop_info(vop_fs(node), sfs);
	struct sfs_inode *sin = vop_info(node, sfs_inode);
	int ret;
	lock_sfs_mutex(sfs);
	{
		if ((ret = trylock_sin(sin)) == 0) {
			ret = sfs_unlink_nolock(sfs, sin, name);
			unlock_sin(sin);
		}
	}
	unlock_sfs_mutex(sfs);
	return ret;
}

static char *sfs_lookup_subpath(char *path)
{
	if ((path = strchr(path, '/')) != NULL) {
		while (*path == '/') {
			*path++ = '\0';
		}
		if (*path == '\0') {
			return NULL;
		}
	}
	return path;
}

static int sfs_lookup(struct inode *node, char *path, struct inode **node_store)
{
	struct sfs_fs *sfs = fsop_info(vop_fs(node), sfs);
	assert(*path != '\0' && *path != '/');
	vop_ref_inc(node);
	do {
		struct sfs_inode *sin = vop_info(node, sfs_inode);
		if (sin->din->type != SFS_TYPE_DIR) {
			vop_ref_dec(node);
			return -E_NOTDIR;
		}

		char *subpath;
next:
		subpath = sfs_lookup_subpath(path);
		if (strcmp(path, ".") == 0) {
			if ((path = subpath) != NULL) {
				goto next;
			}
			break;
		}

		int ret;
		struct inode *subnode;
		if (strcmp(path, "..") == 0) {
			ret = sfs_load_parent(sfs, sin, &subnode);
		} else {
			if (strlen(path) > SFS_MAX_FNAME_LEN) {
				vop_ref_dec(node);
				return -E_TOO_BIG;
			}
			ret = sfs_lookup_once(sfs, sin, path, &subnode, NULL);
		}

		vop_ref_dec(node);
		if (ret != 0) {
			return ret;
		}
		node = subnode, path = subpath;
	} while (path != NULL);
	*node_store = node;
	return 0;
}

static int
sfs_lookup_parent(struct inode *node, char *path, struct inode **node_store,
		  char **endp)
{
	struct sfs_fs *sfs = fsop_info(vop_fs(node), sfs);
	assert(*path != '\0' && *path != '/');
	vop_ref_inc(node);
	while (1) {
		struct sfs_inode *sin = vop_info(node, sfs_inode);
		if (sin->din->type != SFS_TYPE_DIR) {
			vop_ref_dec(node);
			return -E_NOTDIR;
		}

		char *subpath;
next:
		if ((subpath = sfs_lookup_subpath(path)) == NULL) {
			*node_store = node, *endp = path;
			return 0;
		}
		if (strcmp(path, ".") == 0) {
			path = subpath;
			goto next;
		}

		int ret;
		struct inode *subnode;
		if (strcmp(path, "..") == 0) {
			ret = sfs_load_parent(sfs, sin, &subnode);
		} else {
			if (strlen(path) > SFS_MAX_FNAME_LEN) {
				vop_ref_dec(node);
				return -E_TOO_BIG;
			}
			ret = sfs_lookup_once(sfs, sin, path, &subnode, NULL);
		}

		vop_ref_dec(node);
		if (ret != 0) {
			return ret;
		}
		node = subnode, path = subpath;
	}
}

static const struct inode_ops sfs_node_dirops = {
	.vop_magic = VOP_MAGIC,
	.vop_open = sfs_opendir,
	.vop_close = sfs_close,
	.vop_read = NULL_VOP_ISDIR,
	.vop_write = NULL_VOP_ISDIR,
	.vop_fstat = sfs_fstat,
	.vop_fsync = sfs_fsync,
	.vop_mkdir = sfs_mkdir,
	.vop_link = sfs_link,
	.vop_rename = sfs_rename,
	.vop_readlink = NULL_VOP_ISDIR,
	.vop_symlink = NULL_VOP_UNIMP,
	.vop_namefile = sfs_namefile,
	.vop_getdirentry = sfs_getdirentry,
	.vop_reclaim = sfs_reclaim,
	.vop_ioctl = NULL_VOP_INVAL,
	.vop_gettype = sfs_gettype,
	.vop_tryseek = NULL_VOP_ISDIR,
	.vop_truncate = NULL_VOP_ISDIR,
	.vop_create = sfs_create,
	.vop_unlink = sfs_unlink,
	.vop_lookup = sfs_lookup,
	.vop_lookup_parent = sfs_lookup_parent,
};

static const struct inode_ops sfs_node_fileops = {
	.vop_magic = VOP_MAGIC,
	.vop_open = sfs_openfile,
	.vop_close = sfs_close,
	.vop_read = sfs_read,
	.vop_write = sfs_write,
	.vop_fstat = sfs_fstat,
	.vop_fsync = sfs_fsync,
	.vop_mkdir = NULL_VOP_NOTDIR,
	.vop_link = NULL_VOP_NOTDIR,
	.vop_rename = NULL_VOP_NOTDIR,
	.vop_readlink = NULL_VOP_NOTDIR,
	.vop_symlink = NULL_VOP_NOTDIR,
	.vop_namefile = NULL_VOP_NOTDIR,
	.vop_getdirentry = NULL_VOP_NOTDIR,
	.vop_reclaim = sfs_reclaim,
	.vop_ioctl = NULL_VOP_INVAL,
	.vop_gettype = sfs_gettype,
	.vop_tryseek = sfs_tryseek,
	.vop_truncate = sfs_truncfile,
	.vop_create = NULL_VOP_NOTDIR,
	.vop_unlink = NULL_VOP_NOTDIR,
	.vop_lookup = NULL_VOP_NOTDIR,
	.vop_lookup_parent = NULL_VOP_NOTDIR,
};
