#include <ulib.h>
#include <stdio.h>
#include <thread.h>

const int forknum = 125;

void do_yield(void)
{
	int i;
	for (i = 0; i < 30; i++) {
		sleep(10);
		yield();
	}
}

void process_main(void)
{
	do_yield();
}

int thread_main(void *arg)
{
	int i, pid;
	for (i = 0; i < forknum; i++) {
		if ((pid = fork()) == 0) {
			process_main();
			exit(0);
		}
	}
	do_yield();
	return 0;
}

int main(void)
{
	thread_t tids[10];

	int i, n = sizeof(tids) / sizeof(tids[0]);
	for (i = 0; i < n; i++) {
		if (thread(thread_main, NULL, tids + i) != 0) {
			goto failed;
		}
	}

	int count = 0;
	while (wait() == 0) {
		count++;
	}

	assert(count == (forknum + 1) * n);
	cprintf("threadfork pass.\n");
	return 0;

failed:
	for (i = 0; i < n; i++) {
		if (tids[i].pid > 0) {
			kill(tids[i].pid);
		}
	}
	panic("FAIL: T.T\n");
}
