#include <ulib.h>
#include <stdio.h>
#include <thread.h>

int thread_loop(void *arg)
{
	int quit = (long)arg;
	while (quit != 0)
		/* do nothing */ ;

	int i;
	for (i = 0; i < 3; i++) {
		cprintf("yield %d.\n", i);
		yield();
	}
	cprintf("exit thread group now.\n");
	exit(0);
}

int main(void)
{
	thread_t tids[10];

	int i, n = sizeof(tids) / sizeof(tids[0]);
	for (i = 0; i < n; i++) {
		if (thread(thread_loop, (void *)(long)i, tids + i) != 0) {
			goto failed;
		}
	}

	cprintf("thread ok.\n");

	while (1)
		/* do nothing */ ;

	return 0;

failed:
	for (i = 0; i < 10; i++) {
		if (tids[i].pid > 0) {
			kill(tids[i].pid);
		}
	}
	panic("FAIL: T.T\n");
}
