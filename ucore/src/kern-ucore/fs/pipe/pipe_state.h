#ifndef __KERN_FS_PIPE_PIPE_STATE_H__
#define __KERN_FS_PIPE_PIPE_STATE_H__

struct pipe_state;

struct pipe_state *pipe_state_create(void);
void pipe_state_acquire(struct pipe_state *state);
void pipe_state_release(struct pipe_state *state);
void pipe_state_close(struct pipe_state *state);

size_t pipe_state_size(struct pipe_state *state, bool write);
size_t pipe_state_read(struct pipe_state *state, void *buf, size_t n);
size_t pipe_state_write(struct pipe_state *state, void *buf, size_t n);

#endif /* !__KERN_FS_PIPE_PIPE_STATE_H__ */
