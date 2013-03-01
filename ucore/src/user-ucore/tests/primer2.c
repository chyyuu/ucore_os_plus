#include <ulib.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>

const int total = 1000;
int *note;

void *safe_shmem_malloc(size_t size)
{
	void *ret;
	if ((ret = shmem_malloc(size)) == NULL) {
		exit(-0xdead);
	}
	return ret;
}

int __read(int index, sem_t sem[])
{
	int ret;
	if (sem_wait(sem[0]) != 0) {
		return -1;
	}
	ret = note[index];
	if (sem_post(sem[1]) != 0) {
		return -1;
	}
	return ret;
}

int __write(int index, sem_t sem[], int val)
{
	int ret;
	if (sem_wait(sem[1]) != 0) {
		return -1;
	}
	ret = note[index], note[index] = val;
	if (sem_post(sem[0]) != 0) {
		return -1;
	}
	return (ret >= 0) ? 0 : -1;
}

void read_and_quit(int index, sem_t sem[])
{
	sem_wait(sem[0]);
	note[index] = -1;
	sem_post(sem[1]);
}

void primeproc(sem_t sem[])
{
	int index = 0, this, num, pid = 0;
	sem_t next_sem[2];
top:
	this = __read(index, sem);
	cprintf("%d is a primer.\n", this);

	while ((num = __read(index, sem)) > 0) {
		if ((num % this) == 0) {
			continue;
		}
		if (pid == 0) {
			if (index + 1 == total) {
				goto out;
			}
			if ((next_sem[0] = sem_init(0)) < 0
			    || (next_sem[1] = sem_init(1)) < 0) {
				goto out;
			}
			if ((pid = fork()) == 0) {
				sem[0] = next_sem[0];
				sem[1] = next_sem[1];
				index++;
				goto top;
			}
			if (pid < 0) {
				goto out;
			}
		}
		if (__write(index + 1, next_sem, num) != 0) {
			goto out;
		}
	}

out:
	cprintf("[%04d] %d quit.\n", getpid(), index);
	read_and_quit(index, sem);
}

int main(void)
{
	note = safe_shmem_malloc(total * sizeof(int));
	memset(note, 0, sizeof(int) * total);
	cprintf("sharemem init ok.\n");

	sem_t sem[2];
	assert((sem[0] = sem_init(0)) > 0 && (sem[1] = sem_init(1)) > 0);

	unsigned int time = gettime_msec();

	int pid;
	if ((pid = fork()) == 0) {
		primeproc(sem);
		exit(0);
	}
	assert(pid > 0);

	int i;
	for (i = 2;; i++) {
		if (__write(0, sem, i) != 0) {
			break;
		}
	}

	cprintf("use %d msecs.\n", gettime_msec() - time);
	cprintf("primer2 pass.\n");
	return 0;
}
