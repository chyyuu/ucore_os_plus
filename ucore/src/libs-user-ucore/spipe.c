#include <types.h>
#include <unistd.h>
#include <ulib.h>
#include <spipe.h>

#define SPIPE_SIZE      4096
#define SPIPE_BUFSIZE   (SPIPE_SIZE - sizeof(__spipe_state_t))

static bool __spipeisclosed_nolock(spipe_t * p, bool read);

int spipe(spipe_t * p)
{
	static_assert(SPIPE_SIZE > SPIPE_BUFSIZE);
	int ret;
	sem_t spipe_sem;
	uintptr_t addr = 0;
	if ((ret = shmem(&addr, SPIPE_SIZE, MMAP_WRITE)) != 0) {
		goto failed;
	}
	if ((spipe_sem = sem_init(1)) < 0) {
		goto failed_cleanup_mem;
	}
	p->isclosed = 0;
	p->spipe_sem = spipe_sem;
	p->addr = addr;
	p->state = (__spipe_state_t *) addr;
	p->buf = (uint8_t *) (p->state + 1);
	p->state->p_rpos = p->state->p_wpos = 0;
	p->state->isclosed = 0;
out:
	return ret;

failed_cleanup_mem:
	munmap(addr, SPIPE_BUFSIZE);
failed:
	goto out;
}

size_t spiperead(spipe_t * p, void *buf, size_t n)
{
	size_t ret = 0;
	__spipe_state_t *state = p->state;
try_again:
	if (p->isclosed || sem_wait(p->spipe_sem) != 0) {
		goto out;
	}
	if (__spipeisclosed_nolock(p, 1)) {
		goto out_unlock;
	}
	if (state->p_rpos == state->p_wpos) {
		sem_post(p->spipe_sem);
		yield();
		goto try_again;
	}
	for (; ret < n; ret++, state->p_rpos++) {
		if (state->p_rpos == state->p_wpos) {
			break;
		}
		*(uint8_t *) (buf + ret) =
		    p->buf[state->p_rpos % SPIPE_BUFSIZE];
	}
out_unlock:
	sem_post(p->spipe_sem);
out:
	return ret;
}

size_t spipewrite(spipe_t * p, void *buf, size_t n)
{
	size_t ret = 0;
	__spipe_state_t *state = p->state;
try_again:
	if (p->isclosed || sem_wait(p->spipe_sem) != 0) {
		goto out;
	}
	if (__spipeisclosed_nolock(p, 0)) {
		goto out_unlock;
	}
	for (; ret < n; ret++, state->p_wpos++) {
		if (state->p_wpos - state->p_rpos >= SPIPE_BUFSIZE) {
			sem_post(p->spipe_sem);
			yield();
			goto try_again;
		}
		p->buf[state->p_wpos % SPIPE_BUFSIZE] =
		    *(uint8_t *) (buf + ret);
	}
out_unlock:
	sem_post(p->spipe_sem);
out:
	return ret;
}

static int __spipeclose_nolock(spipe_t * p)
{
	if (!p->isclosed) {
		p->isclosed = p->state->isclosed = 1;
		munmap(p->addr, SPIPE_BUFSIZE);
		return 0;
	}
	return -1;
}

int spipeclose(spipe_t * p)
{
	if (!p->isclosed) {
		int ret;
		sem_wait(p->spipe_sem);
		ret = __spipeclose_nolock(p);
		sem_post(p->spipe_sem);
		return ret;
	}
	return -1;
}

static bool __spipeisclosed_nolock(spipe_t * p, bool read)
{
	if (!p->isclosed) {
		if (!p->state->isclosed) {
			return 0;
		}
		if (p->state->p_rpos < p->state->p_wpos) {
			return !read;
		}
		__spipeclose_nolock(p);
	}
	return 1;
}

bool spipeisclosed(spipe_t * p)
{
	if (!p->isclosed) {
		bool isclosed;
		sem_wait(p->spipe_sem);
		isclosed = __spipeisclosed_nolock(p, 0);
		sem_post(p->spipe_sem);
		return isclosed;
	}
	return 1;
}
