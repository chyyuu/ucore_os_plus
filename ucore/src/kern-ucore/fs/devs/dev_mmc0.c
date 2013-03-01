#include <types.h>
#include <mmu.h>
#include <slab.h>
#include <sem.h>
#include <ide.h>
#include <inode.h>
#include <dev.h>
#include <vfs.h>
#include <iobuf.h>
#include <error.h>
#include <assert.h>

#define MMC0_BLKSIZE                   PGSIZE
#define MMC0_BUFSIZE                   (4 * MMC0_BLKSIZE)
#define MMC0_BLK_NSECT                 (MMC0_BLKSIZE / SECTSIZE)

static char *mmc0_buffer;
static semaphore_t mmc0_sem;

static void lock_mmc0(void)
{
	down(&(mmc0_sem));
}

static void unlock_mmc0(void)
{
	up(&(mmc0_sem));
}

static int mmc0_open(struct device *dev, uint32_t open_flags)
{
	return 0;
}

static int mmc0_close(struct device *dev)
{
	return 0;
}

static void mmc0_read_blks_nolock(uint32_t blkno, uint32_t nblks)
{
	int ret;
	uint32_t sectno = blkno * MMC0_BLK_NSECT, nsecs =
	    nblks * MMC0_BLK_NSECT;
	if ((ret = ide_read_secs(MMC0_DEV_NO, sectno, mmc0_buffer, nsecs)) != 0) {
		panic
		    ("mmc0: read blkno = %d (sectno = %d), nblks = %d (nsecs = %d): 0x%08x.\n",
		     blkno, sectno, nblks, nsecs, ret);
	}
}

static void mmc0_write_blks_nolock(uint32_t blkno, uint32_t nblks)
{
	int ret;
	uint32_t sectno = blkno * MMC0_BLK_NSECT, nsecs =
	    nblks * MMC0_BLK_NSECT;
	if ((ret =
	     ide_write_secs(MMC0_DEV_NO, sectno, mmc0_buffer, nsecs)) != 0) {
		panic
		    ("mmc0: write blkno = %d (sectno = %d), nblks = %d (nsecs = %d): 0x%08x.\n",
		     blkno, sectno, nblks, nsecs, ret);
	}
}

static int mmc0_io(struct device *dev, struct iobuf *iob, bool write)
{
	off_t offset = iob->io_offset;
	size_t resid = iob->io_resid;
	uint32_t blkno = offset / MMC0_BLKSIZE;
	uint32_t nblks = resid / MMC0_BLKSIZE;

	/* don't allow I/O that isn't block-aligned */
	if ((offset % MMC0_BLKSIZE) != 0 || (resid % MMC0_BLKSIZE) != 0) {
		return -E_INVAL;
	}

	/* don't allow I/O past the end of mmc0 */
	if (blkno + nblks > dev->d_blocks) {
		return -E_INVAL;
	}

	/* read/write nothing ? */
	if (nblks == 0) {
		return 0;
	}

	lock_mmc0();
	while (resid != 0) {
		size_t copied, alen = MMC0_BUFSIZE;
		if (write) {
			iobuf_move(iob, mmc0_buffer, alen, 0, &copied);
			assert(copied != 0 && copied <= resid
			       && copied % MMC0_BLKSIZE == 0);
			nblks = copied / MMC0_BLKSIZE;
			mmc0_write_blks_nolock(blkno, nblks);
		} else {
			if (alen > resid) {
				alen = resid;
			}
			nblks = alen / MMC0_BLKSIZE;
			mmc0_read_blks_nolock(blkno, nblks);
			iobuf_move(iob, mmc0_buffer, alen, 1, &copied);
			assert(copied == alen && copied % MMC0_BLKSIZE == 0);
		}
		resid -= copied, blkno += nblks;
	}
	unlock_mmc0();
	return 0;
}

static int mmc0_ioctl(struct device *dev, int op, void *data)
{
	return -E_UNIMP;
}

static void mmc0_device_init(struct device *dev)
{
	memset(dev, 0, sizeof(*dev));
	static_assert(MMC0_BLKSIZE % SECTSIZE == 0);
	if (!ide_device_valid(MMC0_DEV_NO)) {
		panic("mmc0 device isn't available.\n");
	}
	dev->d_blocks = ide_device_size(MMC0_DEV_NO) / MMC0_BLK_NSECT;
	dev->d_blocksize = MMC0_BLKSIZE;
	dev->d_open = mmc0_open;
	dev->d_close = mmc0_close;
	dev->d_io = mmc0_io;
	dev->d_ioctl = mmc0_ioctl;
	sem_init(&(mmc0_sem), 1);

	static_assert(MMC0_BUFSIZE % MMC0_BLKSIZE == 0);
	if ((mmc0_buffer = kmalloc(MMC0_BUFSIZE)) == NULL) {
		panic("mmc0 alloc buffer failed.\n");
	}
}

void dev_init_mmc0(void)
{
	struct inode *node;
	if ((node = dev_create_inode()) == NULL) {
		panic("mmc0: dev_create_node.\n");
	}
	mmc0_device_init(vop_info(node, device));

	int ret;
	if ((ret = vfs_add_dev("mmc0", node, 1)) != 0) {
		panic("mmc0: vfs_add_dev: %e.\n", ret);
	}
}
