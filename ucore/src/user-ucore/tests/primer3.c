#include <ulib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

const int total = 1000;

void primeproc(void)
{
	int index = 0, this, num, pid = 0;
top:
	recv_event(NULL, &this);
	cprintf("%d is a primer.\n", this);

	while (recv_event(NULL, &num) == 0) {
		if ((num % this) == 0) {
			continue;
		}
		if (pid == 0) {
			if (index + 1 == total) {
				goto out;
			}
			if ((pid = fork()) == 0) {
				index++;
				goto top;
			}
			if (pid < 0) {
				goto out;
			}
		}
		if (send_event(pid, num) != 0) {
			goto out;
		}
	}
out:
	cprintf("[%04d] %d quit.\n", getpid(), index);
}

int main(void)
{
	int i, pid;
	unsigned int time = gettime_msec();
	if ((pid = fork()) == 0) {
		primeproc();
		exit(0);
	}
	assert(pid > 0);

	for (i = 2;; i++) {
		if (send_event(pid, i) != 0) {
			break;
		}
	}
	cprintf("use %d msecs.\n", gettime_msec() - time);
	cprintf("primer3 pass.\n");
	return 0;
}
