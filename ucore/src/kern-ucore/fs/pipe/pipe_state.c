#include <types.h>
#include <wait.h>
#include <slab.h>
#include <mmu.h>
#include <sync.h>
#include <proc.h>
#include <sched.h>
#include <sem.h>
#include <atomic.h>
#include <pipe.h>
#include <pipe_state.h>
#include <error.h>
#include <assert.h>

struct pipe_state {
	off_t p_rpos;
	off_t p_wpos;
	uint8_t *buf;
	bool isclosed;
	int ref_count;
	semaphore_t sem;
	wait_queue_t reader_queue;
	wait_queue_t writer_queue;
};

#define PIPE_BUFSIZE                            (PGSIZE - sizeof(struct pipe_state))

struct pipe_state *pipe_state_create(void)
{
	static_assert((int)PIPE_BUFSIZE > 128);
	struct pipe_state *state;
	if ((state = kmalloc(sizeof(struct pipe_state) + PIPE_BUFSIZE)) != NULL) {
		state->p_rpos = state->p_wpos = 0;
		state->buf = (uint8_t *) (state + 1);
		state->isclosed = 0;
		state->ref_count = 1;
		sem_init(&(state->sem), 1);
		wait_queue_init(&(state->reader_queue));
		wait_queue_init(&(state->writer_queue));
	}
	return state;
}

static void lock_state(struct pipe_state *state)
{
	down(&(state->sem));
}

static void unlock_state(struct pipe_state *state)
{
	up(&(state->sem));
}

static inline bool is_empty(struct pipe_state *state)
{
	return state->p_rpos == state->p_wpos;
}

static inline bool is_full(struct pipe_state *state)
{
	return state->p_wpos - state->p_rpos >= PIPE_BUFSIZE;
}

static bool pipe_state_wait(wait_queue_t * queue)
{
	bool intr_flag;
	wait_t __wait, *wait = &__wait;
	local_intr_save(intr_flag);
	wait_current_set(queue, wait, WT_PIPE);
	local_intr_restore(intr_flag);

	schedule();

	local_intr_save(intr_flag);
	wait_current_del(queue, wait);
	local_intr_restore(intr_flag);
	return wait->wakeup_flags == WT_PIPE;
}

static void pipe_state_wakeup(wait_queue_t * queue)
{
	if (!wait_queue_empty(queue)) {
		bool intr_flag;
		local_intr_save(intr_flag);
		{
			wakeup_queue(queue, WT_PIPE, 1);
		}
		local_intr_restore(intr_flag);
	}
}

#define wait_reader(state)                          pipe_state_wait(&((state)->writer_queue))
#define wait_writer(state)                          pipe_state_wait(&((state)->reader_queue))
#define wakeup_reader(state)                        pipe_state_wakeup(&((state)->reader_queue))
#define wakeup_writer(state)                        pipe_state_wakeup(&((state)->writer_queue))

void pipe_state_acquire(struct pipe_state *state)
{
	assert(state->ref_count > 0);
	state->ref_count++;
}

void pipe_state_release(struct pipe_state *state)
{
	assert(state != NULL && state->ref_count > 0);
	if (--state->ref_count == 0) {
		assert(wait_queue_empty(&(state->reader_queue)));
		assert(wait_queue_empty(&(state->writer_queue)));
		kfree(state);
	}
}

void pipe_state_close(struct pipe_state *state)
{
	assert(state != NULL && state->ref_count > 0);
	state->isclosed = 1;
	wakeup_reader(state);
	wakeup_writer(state);
}

size_t pipe_state_size(struct pipe_state *state, bool write)
{
	size_t size = state->p_wpos - state->p_rpos;
	if (write) {
		if (state->isclosed) {
			return 0;
		}
		return PIPE_BUFSIZE - size;
	}
	return size;
}

size_t pipe_state_read(struct pipe_state * state, void *buf, size_t n)
{
	size_t ret = 0;
try_again:
	lock_state(state);
	if (is_empty(state)) {
		if (state->isclosed) {
			goto out_unlock;
		} else {
			unlock_state(state);
			if (!wait_writer(state)) {
				goto out;
			}
			goto try_again;
		}
	}
	for (; ret < n && !is_empty(state); ret++, state->p_rpos++) {
		*(uint8_t *) (buf + ret) =
		    state->buf[state->p_rpos % PIPE_BUFSIZE];
	}
	if (ret != 0) {
		wakeup_writer(state);
	}

out_unlock:
	unlock_state(state);
out:
	return ret;
}

size_t pipe_state_write(struct pipe_state * state, void *buf, size_t n)
{
	size_t ret = 0, step;
try_again:
	lock_state(state);
	if (state->isclosed) {
		goto out_unlock;
	}
	for (step = 0; ret < n; ret++, step++, state->p_wpos++) {
		if (is_full(state)) {
			wakeup_reader(state);
			unlock_state(state);
			if (!wait_reader(state)) {
				goto out;
			}
			goto try_again;
		}
		state->buf[state->p_wpos % PIPE_BUFSIZE] =
		    *(uint8_t *) (buf + ret);
	}
	if (step != 0) {
		wakeup_reader(state);
	}

out_unlock:
	unlock_state(state);
out:
	return ret;
}
