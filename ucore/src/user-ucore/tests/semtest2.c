#include <ulib.h>
#include <stdio.h>

sem_t mutex;

void test1(void)
{
	cprintf("semtest2 test1:\n");
	int nloop = 3, i, pid;
	if ((pid = fork()) == 0) {
		for (i = 0; i < nloop; i++) {
			sem_wait(mutex);
			cprintf("child start %d.\n", i);
			sleep(50);
			cprintf("child end.\n");
			sem_post(mutex);
		}
		cprintf("child exit.\n");
		exit(0xbad);
	}
	assert(pid > 0);
	yield();
	for (i = 0; i < nloop; i++) {
		sem_wait(mutex);
		cprintf("parent start %d.\n", i);
		sleep(50);
		cprintf("parent end.\n");
		sem_post(mutex);
	}
	assert(wait() == 0);
}

void test2(void)
{
	cprintf("semtest2 test2:\n");
	int nloop = 3, i, pid;
	if ((pid = fork()) == 0) {
		sem_wait(mutex);
		for (i = 0; i < nloop; i++) {
			sem_wait(mutex);
			cprintf("child %d\n", i);
			sleep(50 * (i + 1));
			sem_post(mutex);
		}
		sem_post(mutex);
		exit(0xbad);
	}
	assert(pid > 0);
	sleep(30);
	for (i = 0; i < nloop; i++) {
		sem_post(mutex);
		yield();
		cprintf("parent %d\n", i);
		sem_wait(mutex);
	}
	assert(wait() == 0);
}

int main(void)
{
	assert((mutex = sem_init(1)) > 0);
	test1();
	test2();
	cprintf("semtest2 pass.\n");
	return 0;
}
