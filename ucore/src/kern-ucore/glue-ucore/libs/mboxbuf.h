#ifndef __LIBS_MBOXBUF_H__
#define __LIBS_MBOXBUF_H__

#include <types.h>

#define MAX_MSG_SLOTS               0x1000
#define MAX_MSG_BYTES               0x10000

struct mboxbuf {
	int from;
	size_t len;
	size_t size;
	void *data;
};

struct mboxinfo {
	size_t slots;
	size_t max_slots;
	bool inuse;
	bool has_sender;
	bool has_receiver;
};

#endif /* !__LIBS_MBOXBUF_H__ */
