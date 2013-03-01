#include <stdio.h>
#include <ulib.h>
#include <malloc.h>

struct slot {
	char data[4096];
	struct slot *next;
};

void glutton(void)
{
	struct slot *tmp, *head = NULL;
	int n = 0;
	cprintf("I am child and I will eat out all the memory.\n");
	while ((tmp = (struct slot *)malloc(sizeof(struct slot))) != NULL) {
		if ((++n) % 1000 == 0) {
			cprintf("I ate %d slots.\n", n);
			sleep(50);
		}
		tmp->next = head;
		head = tmp;
	}
	exit(0xdead);
}

void sleepy(int pid)
{
	int i, time = 100;
	for (i = 0; i < 10; i++) {
		sleep(time);
		cprintf("sleep %d x %d slices.\n", i + 1, time);
	}
	assert(kill(pid) == 0);
	exit(0);
}

int main(void)
{
	unsigned int time = gettime_msec();
	int pid1, pid2, exit_code;
	if ((pid1 = fork()) == 0) {
		glutton();
	}
	assert(pid1 > 0);

	if ((pid2 = fork()) == 0) {
		sleepy(pid1);
	}
	if (pid2 < 0) {
		kill(pid1);
		panic("fork failed.\n");
	}

	assert(waitpid(pid2, &exit_code) == 0 && exit_code == 0);
	cprintf("use %04d msecs.\n", gettime_msec() - time);
	cprintf("sleep pass.\n");
	return 0;
}
