#ifndef __KERN_SYNC_MBOX_H__
#define __KERN_SYNC_MBOX_H__

#include <types.h>

void mbox_init(void);

struct mboxbuf;
struct mboxinfo;

int ipc_mbox_init(unsigned int max_slots);
int ipc_mbox_send(int id, struct mboxbuf *buf, unsigned int timeout);
int ipc_mbox_recv(int id, struct mboxbuf *buf, unsigned int timeout);
int ipc_mbox_free(int id);
int ipc_mbox_info(int id, struct mboxinfo *info);

void mbox_cleanup(void);

#endif /* !__KERN_SYNC_MBOX_H__ */
