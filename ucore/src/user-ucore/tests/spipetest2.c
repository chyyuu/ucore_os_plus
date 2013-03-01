#include <stdio.h>
#include <ulib.h>
#include <string.h>
#include <spipe.h>
#include <malloc.h>
#include <string.h>
#include <thread.h>

static spipe_t pipe;

thread_t tids[10];
int total = sizeof(tids) / sizeof(tids[0]);

int thread_main(void *arg)
{
	long id = (long)arg;
	cprintf("this is %d\n", id);

	size_t n = 10000;
	char *buf = malloc(sizeof(char) * n);
	if (buf == NULL) {
		return -1;
	}

	memset(buf, (char)id, n);

	int i, rounds = 20;
	for (i = 0; i < rounds; i++) {
		size_t ret = spipewrite(&pipe, buf, n);
		if (ret != n) {
			cprintf("pipe is closed, too early.\n");
			return -1;
		}
		if (id == 0) {
			cprintf("send %d/%d\n", i, rounds);
		}
	}
	return 0;
}

void process_main(void)
{
	int counts[total], i;
	for (i = 0; i < total; i++) {
		counts[i] = 0;
	}

	char buf[128];
	size_t n = sizeof(buf);

	while (1) {
		size_t ret = spiperead(&pipe, buf, n);
		if (ret == 0) {
			break;
		}
		for (i = 0; i < ret; i++) {
			counts[((unsigned int)buf[i]) % total]++;
		}
	}
	if (spipeisclosed(&pipe)) {
		for (i = 0; i < total; i++) {
			cprintf("%d reads %d\n", i, counts[i]);
		}
		exit(0);
	}
	exit(0xbad);
}

int main(void)
{
	int pid, i;
	assert(spipe(&pipe) == 0);

	if ((pid = fork()) == 0) {
		process_main();
	}
	assert(pid > 0);

	memset(tids, 0, sizeof(thread_t) * total);
	for (i = 0; i < total; i++) {
		if (thread(thread_main, (void *)(long)i, tids + i) != 0) {
			goto failed;
		}
	}

	int exit_code;
	for (i = 0; i < total; i++) {
		if (thread_wait(tids + i, &exit_code) != 0 || exit_code != 0) {
			goto failed;
		}
	}

	for (i = 0; i < total; i++) {
		yield();
	}

	assert(spipeclose(&pipe) == 0);
	assert(waitpid(pid, &exit_code) == 0 && exit_code == 0);
	cprintf("spipetest2 pass.\n");
	return 0;

failed:
	for (i = 0; i < total; i++) {
		if (tids[i].pid > 0) {
			kill(tids[i].pid);
		}
	}
	kill(pid);
	panic("FAIL: T.T\n");
}
