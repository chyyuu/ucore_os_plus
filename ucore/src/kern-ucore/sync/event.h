#ifndef __KERN_SYNC_EVENT_H__
#define __KERN_SYNC_EVENT_H__

#include <types.h>
#include <wait.h>

typedef struct {
	int event;
	wait_queue_t wait_queue;
} event_t;

void event_box_init(event_t * event_box);

int ipc_event_send(int pid, int event, unsigned int timeout);
int ipc_event_recv(int *pid_store, int *event_store, unsigned int timeout);

#endif /* !__KERN_SYNC_EVENT_H__ */
