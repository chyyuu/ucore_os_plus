#include <ulib.h>
#include <stdio.h>

sem_t mutex;

int main(void)
{
	assert((mutex = sem_init(1)) > 0);
	assert(sem_post(mutex) == 0);

	int value, pid;
	assert(sem_get_value(mutex, &value) == 0 && value == 2);
	assert(sem_free(mutex) == 0 && sem_get_value(mutex, &value) != 0);

	assert((mutex = sem_init(0)) > 0);
	if ((pid = fork()) == 0) {
		assert(sem_wait(mutex) != 0);
		cprintf("child exit ok.\n");
		exit(0);
	}
	assert(pid > 0);
	assert(sem_free(mutex) == 0 && waitpid(pid, &value) == 0 && value == 0);
	cprintf("semtest4 pass.\n");
	return 0;
}
