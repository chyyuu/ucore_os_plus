#include <ulib.h>
#include <stdio.h>

const int max_child = 32;

int main(void)
{
	int n, pid;
	for (n = 0; n < max_child; n++) {
		pid = fork();
		if (pid == 0) {
			cprintf("I am child %d\n", n);
			exit(0);
		}
		assert(pid > 0);
	}

	if (n > max_child) {
		panic("%d fork claimed to work %d times!\n", getpid(), n);
	}

	for (; n > 0; n--) {
		if (wait() != 0) {
			panic("wait stopped early %d\n", getpid());
		}
	}

	if (wait() == 0) {
		panic("wait got too many\n");
	}

	cprintf("forktest pass %d.\n", getpid());
	return 0;
}
