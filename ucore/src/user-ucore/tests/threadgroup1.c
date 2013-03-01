#include <ulib.h>
#include <stdio.h>
#include <thread.h>

int thread_loop(void *arg)
{
	while (1)
		/* do nothing */ ;
}

int main(void)
{
	thread_t tids[10];

	int i, n = sizeof(tids) / sizeof(tids[0]);
	for (i = 0; i < n; i++) {
		if (thread(thread_loop, NULL, tids + i) != 0) {
			goto failed;
		}
	}

	cprintf("thread ok.\n");

	for (i = 0; i < 3; i++) {
		cprintf("yield %d.\n", i);
		yield();
	}
	cprintf("exit thread group now.\n");
	return 0;

failed:
	for (i = 0; i < 10; i++) {
		if (tids[i].pid > 0) {
			kill(tids[i].pid);
		}
	}
	panic("FAIL: T.T\n");
}
