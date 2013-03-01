#ifndef __USER_LIBS_SPIPE_H__
#define __USER_LIBS_SPIPE_H__

#include <types.h>

typedef struct {
	off_t p_rpos;
	off_t p_wpos;
	bool isclosed;
} __spipe_state_t;

typedef struct {
	volatile bool isclosed;
	sem_t spipe_sem;
	uintptr_t addr;
	__spipe_state_t *state;
	uint8_t *buf;
} spipe_t;

int spipe(spipe_t * p);
size_t spiperead(spipe_t * p, void *buf, size_t n);
size_t spipewrite(spipe_t * p, void *buf, size_t n);
int spipeclose(spipe_t * p);
bool spipeisclosed(spipe_t * p);

#endif /* !__USER_LIBS_SPIPE_H__ */
