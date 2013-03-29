#include <ulib.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <lock.h>

int total = 1000;

int *note;
lock_t *locks;

static void *safe_shmem_malloc(size_t size)
{
	void *ret;
	if ((ret = shmem_malloc(size)) == NULL) {
		exit(-0xdead);
	}
	return ret;
}

static int read(int index)
{
	lock_t *l = locks + index;
	int ret;
try_again:
	lock(l);
	if ((ret = note[index]) > 0) {
		note[index] = 0;
	}
	unlock(l);
	if (ret == 0) {
		yield();
		goto try_again;
	}
	return ret;
}

static int write(int index, int val, bool force)
{
	lock_t *l = locks + index;
	int ret;
try_again:
	lock(l);
	if ((ret = note[index]) >= 0) {
		if (ret == 0 || force) {
			note[index] = val;
		}
	}
	unlock(l);
	if (ret > 0 && !force) {
		yield();
		goto try_again;
	}
	return (ret > 0) ? 0 : ret;
}

void primeproc(void)
{
	int index = 0, this, num, pid = 0;
top:
	this = read(index);
	cprintf("%d is a primer.\n", this);

	while ((num = read(index)) > 0) {
		if ((num % this) == 0) {
			continue;
		}
		if (pid == 0) {
			if (index + 1 == total || (pid = fork()) < 0) {
				goto out;
			}
			if (pid == 0) {
				index++;
				goto top;
			}
		}
		if (write(index + 1, num, 0) != 0) {
			goto out;
		}
	}

out:
	cprintf("[%04d] %d quit.\n", getpid(), index);
	write(index, -1, 1);
}

int main(void)
{
	note = safe_shmem_malloc(total * sizeof(int));
	locks = safe_shmem_malloc(total * sizeof(lock_t));

	int i, pid;
	for (i = 0; i < total; i++) {
		note[i] = 0;
		lock_init(locks + i);
	}

	cprintf("sharemem init ok.\n");

	unsigned int time = gettime_msec();

	if ((pid = fork()) == 0) {
		primeproc();
		exit(0);
	}
	assert(pid > 0);

	for (i = 2;; i++) {
		if (write(0, i, 0) != 0) {
			break;
		}
	}

	cprintf("use %d msecs.\n", gettime_msec() - time);
	cprintf("primer pass.\n");
	return 0;
}
