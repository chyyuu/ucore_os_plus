#include <types.h>
#include <string.h>
#include <dev.h>
#include <sfs.h>
#include <iobuf.h>
#include <bitmap.h>
#include <assert.h>

static int
sfs_rwblock_nolock(struct sfs_fs *sfs, void *buf, uint32_t blkno, bool write,
		   bool check)
{
	assert((blkno != 0 || !check) && blkno < sfs->super.blocks);
	struct iobuf __iob, *iob =
	    iobuf_init(&__iob, buf, SFS_BLKSIZE, blkno * SFS_BLKSIZE);
	return dop_io(sfs->dev, iob, write);
}

static int
sfs_rwblock(struct sfs_fs *sfs, void *buf, uint32_t blkno, uint32_t nblks,
	    bool write)
{
	int ret = 0;
	lock_sfs_io(sfs);
	{
		while (nblks != 0) {
			if ((ret =
			     sfs_rwblock_nolock(sfs, buf, blkno, write,
						1)) != 0) {
				break;
			}
			blkno++, nblks--;
			buf += SFS_BLKSIZE;
		}
	}
	unlock_sfs_io(sfs);
	return ret;
}

int sfs_rblock(struct sfs_fs *sfs, void *buf, uint32_t blkno, uint32_t nblks)
{
	return sfs_rwblock(sfs, buf, blkno, nblks, 0);
}

int sfs_wblock(struct sfs_fs *sfs, void *buf, uint32_t blkno, uint32_t nblks)
{
	return sfs_rwblock(sfs, buf, blkno, nblks, 1);
}

int
sfs_rbuf(struct sfs_fs *sfs, void *buf, size_t len, uint32_t blkno,
	 off_t offset)
{
	assert(offset >= 0 && offset < SFS_BLKSIZE
	       && offset + len <= SFS_BLKSIZE);
	int ret;
	lock_sfs_io(sfs);
	{
		if ((ret =
		     sfs_rwblock_nolock(sfs, sfs->sfs_buffer, blkno, 0,
					1)) == 0) {
			memcpy(buf, sfs->sfs_buffer + offset, len);
		}
	}
	unlock_sfs_io(sfs);
	return ret;
}

int
sfs_wbuf(struct sfs_fs *sfs, void *buf, size_t len, uint32_t blkno,
	 off_t offset)
{
	assert(offset >= 0 && offset < SFS_BLKSIZE
	       && offset + len <= SFS_BLKSIZE);
	int ret;
	lock_sfs_io(sfs);
	{
		if ((ret =
		     sfs_rwblock_nolock(sfs, sfs->sfs_buffer, blkno, 0,
					1)) == 0) {
			memcpy(sfs->sfs_buffer + offset, buf, len);
			ret =
			    sfs_rwblock_nolock(sfs, sfs->sfs_buffer, blkno, 1,
					       1);
		}
	}
	unlock_sfs_io(sfs);
	return ret;
}

int sfs_sync_super(struct sfs_fs *sfs)
{
	int ret;
	lock_sfs_io(sfs);
	{
		memset(sfs->sfs_buffer, 0, SFS_BLKSIZE);
		memcpy(sfs->sfs_buffer, &(sfs->super), sizeof(sfs->super));
		ret =
		    sfs_rwblock_nolock(sfs, sfs->sfs_buffer, SFS_BLKN_SUPER, 1,
				       0);
	}
	unlock_sfs_io(sfs);
	return ret;
}

int sfs_sync_freemap(struct sfs_fs *sfs)
{
	uint32_t nblks = sfs_freemap_blocks(&(sfs->super));
	return sfs_wblock(sfs, bitmap_getdata(sfs->freemap, NULL),
			  SFS_BLKN_FREEMAP, nblks);
}

int sfs_clear_block(struct sfs_fs *sfs, uint32_t blkno, uint32_t nblks)
{
	int ret;
	lock_sfs_io(sfs);
	{
		memset(sfs->sfs_buffer, 0, SFS_BLKSIZE);
		while (nblks != 0) {
			if ((ret =
			     sfs_rwblock_nolock(sfs, sfs->sfs_buffer, blkno, 1,
						1)) != 0) {
				break;
			}
			blkno++, nblks--;
		}
	}
	unlock_sfs_io(sfs);
	return ret;
}
