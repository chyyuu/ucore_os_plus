#include <stdio.h>
#include <ulib.h>

void sem_test(void)
{
	sem_t sem_id = sem_init(1);
	assert(sem_id > 0);
	cprintf("sem_id = 0x%016llx\n", sem_id);

	int i, value;
	for (i = 0; i < 10; i++) {
		assert(sem_get_value(sem_id, &value) == 0);
		assert(value == i + 1 && sem_post(sem_id) == 0);
	}
	cprintf("post ok.\n");

	for (; i > 0; i--) {
		assert(sem_wait(sem_id) == 0);
		assert(sem_get_value(sem_id, &value) == 0 && value == i);
	}
	cprintf("wait ok.\n");

	int pid, ret;
	if ((pid = fork()) == 0) {
		assert(sem_get_value(sem_id, &value) == 0);
		assert(value == 1 && sem_wait(sem_id) == 0);

		sleep(10);
		for (i = 0; i < 10; i++) {
			cprintf("sleep %d\n", i);
			sleep(20);
		}
		assert(sem_post(sem_id) == 0);
		exit(0);
	}
	assert(pid > 0);

	sleep(10);
	for (i = 0; i < 10; i++) {
		yield();
	}

	cprintf("wait semaphore...\n");

	assert(sem_wait(sem_id) == 0);
	assert(sem_get_value(sem_id, &value) == 0 && value == 0);
	cprintf("hold semaphore.\n");

	assert(waitpid(pid, &ret) == 0 && ret == 0);
	assert(sem_get_value(sem_id, &value) == 0 && value == 0);
	cprintf("fork pass.\n");
	exit(0);
}

int main(void)
{
	int pid, ret;
	if ((pid = fork()) == 0) {
		sem_test();
	}
	assert(pid > 0 && waitpid(pid, &ret) == 0 && ret == 0);
	cprintf("semtest pass.\n");
	return 0;
}
