#include <ulib.h>
#include <stdio.h>
#include <error.h>

sem_t mutex;

int main(void)
{
	assert((mutex = sem_init(0)) > 0);

	cprintf("wait now...\n");

	assert(sem_wait_timeout(mutex, 500) == -E_TIMEOUT);

	cprintf("wait timeout\n");

	int pid;
	if ((pid = fork()) == 0) {
		cprintf("child now sleep\n");
		sleep(500);
		sem_post(mutex);
		exit(0);
	}
	assert(pid > 0);

	yield();
	assert(sem_wait_timeout(mutex, ~0) == 0);
	cprintf("semtest3 pass.\n");
	return 0;
}
