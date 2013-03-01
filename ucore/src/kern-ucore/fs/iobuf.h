#ifndef __KERN_FS_IOBUF_H__
#define __KERN_FS_IOBUF_H__

#include <types.h>

/*
 * Like BSD uio, but simplified a lot. (In BSD, there can be one or more
 * iovec in a uio.)
 *
 */

struct iobuf {
	void *io_base;		/* The base addr of object       */
	off_t io_offset;	/* Desired offset into object    */
	size_t io_len;		/* The lenght of Data            */
	size_t io_resid;	/* Remaining amt of data to xfer */
};

struct iovec {
	char *iov_base;
	size_t iov_len;
};

/*
 * Copy data from a kernel buffer to a data region defined by a iobuf struct,
 * updating the iobuf struct's offset and resid fields.
 *
 * Before calling this, you should
 *   (1) set up iobuf to point to the buffer you want to transfer
 *       to;
 *   (2) initialize iobuf's offset as desired;
 *   (3) initialize iobuf's resid to the total amount of data that can be 
 *       transferred through this iobuf;
 *
 * After calling, 
 *   (1) the contents of iobuf may be altered and should not be interpreted;
 *   (2) iobuf's offset will have been incremented by the amount transferred;
 *   (3) iobuf's resid will have been decremented by the amount transferred;
 *
 * iobuf_move() may be called repeatedly on the same iobuf to transfer
 * additional data until the available buffer space the iobuf refers to
 * is exhausted.
 *
 * Note that the actual value of iobuf's offset is not interpreted. It is
 * provided to allow for easier file seek pointer management.
 *
 */

#define iobuf_used(iob)                         ((size_t)((iob)->io_len - (iob)->io_resid))

struct iobuf *iobuf_init(struct iobuf *iob, void *base, size_t len,
			 off_t offset);
int iobuf_move(struct iobuf *iob, void *data, size_t len, bool m2b,
	       size_t * copiedp);
/*
 * Like uiomove, but sends zeros.
 */
int iobuf_move_zeros(struct iobuf *iob, size_t len, size_t * copiedp);
void iobuf_skip(struct iobuf *iob, size_t n);

#endif /* !__KERN_FS_IOBUF_H__ */
