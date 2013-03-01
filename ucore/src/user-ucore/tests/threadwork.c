#include <ulib.h>
#include <malloc.h>
#include <thread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char **buffer;
thread_t *tids;
const int size = 47, rounds = 103;

int work(void *arg)
{
	long n = (long)arg;
	int value = hash32(n * n, 8);
	cprintf("i am %02d, %02d, i got %08x\n", n, getpid(), value);
	yield();
	int i, j;
	for (i = 0; i < size; i++) {
		for (j = n; j < size * rounds; j += size) {
			buffer[i][j] = (char)value;
		}
	}
	return 0xbee;
}

int loop(void *arg)
{
	cprintf("child: do nothing\n");
	while (1) ;
}

int main(void)
{
	buffer = (char **)malloc(sizeof(char *) * size);

	int i, j, k, ret;
	for (i = 0; i < size; i++) {
		assert((buffer[i] =
			(char *)malloc(sizeof(char) * size * rounds)) != NULL);
	}

	// print current page table
	print_pgdir();

	assert((tids = (thread_t *) malloc(sizeof(thread_t) * size)) != NULL);
	memset(tids, 0, sizeof(thread_t) * size);

	// create 'size' threads
	for (i = 0; i < size; i++) {
		if ((ret = thread(work, (void *)(long)i, tids + i)) != 0) {
			cprintf("thread %d failed, returns %d\n", i, ret);
			goto failed;
		}
	}

	cprintf("thread ok.\n");

	// yield, make sure that threads have touched their stacks
	for (i = 0; i < size * 2; i++) {
		yield();
	}

	// print current page table
	print_pgdir();

	// wait and check exit codes
	for (i = 0; i < size; i++) {
		int exit_code = 0;
		if (thread_wait(tids + i, &exit_code) != 0) {
			goto failed;
		}
		if (exit_code != 0xbee) {
			cprintf("thread %d exit failed, %d\n", i, exit_code);
			goto failed;
		}
	}

	cprintf("thread wait ok.\n");

	print_pgdir();

	for (k = 0; k < size; k++) {
		int value = hash32(k * k, 8);
		for (i = 0; i < size; i++) {
			for (j = k; j < size * rounds; j += size) {
				assert(buffer[i][j] == (char)value);
			}
		}
	}

	// create a 'loop' thread, kill and wait
	thread_t loop_tid;
	assert(thread(loop, NULL, &loop_tid) == 0);
	cprintf("loop init ok.\n");

	if (thread_kill(&loop_tid) != 0) {
		kill(loop_tid.pid);
		panic("kill loop_tid failed.\n");
	}
	// wait 'loop' quit, wait() == 0
	assert(wait() == 0 && wait() != 0);

	cprintf("threadwork pass.\n");
	return 0;

failed:
	for (i = 0; i < size; i++) {
		if (tids[i].pid > 0) {
			kill(tids[i].pid);
		}
	}
	panic("FAIL: T.T\n");
}
