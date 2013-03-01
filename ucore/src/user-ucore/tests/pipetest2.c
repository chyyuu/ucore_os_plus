#include <stdio.h>
#include <ulib.h>
#include <string.h>
#include <file.h>
#include <malloc.h>
#include <string.h>
#include <thread.h>
#include <unistd.h>

int __fd[2], fd;

thread_t tids[10];
int total = sizeof(tids) / sizeof(tids[0]);

int thread_main(void *arg)
{
	int id = (long)arg;
	cprintf("this is %d\n", id);

	size_t n = 10000;
	char *buf = malloc(sizeof(char) * n);
	if (buf == NULL) {
		return -1;
	}

	memset(buf, (char)id, n);

	int i, rounds = 20, ret;
	for (i = 0; i < rounds; i++) {
		if ((ret = write(fd, buf, n)) < 0 || ret != n) {
			cprintf("pipe is closed, too early.\n");
			return -1;
		}
		if (id == 0) {
			cprintf("send %d/%d\n", i, rounds);
		}
	}
	return 0;
}

void process_main(void)
{
	int counts[total], i, ret;
	for (i = 0; i < total; i++) {
		counts[i] = 0;
	}

	char buf[128];
	size_t n = sizeof(buf);

	while (1) {
		if ((ret = read(fd, buf, n)) <= 0) {
			break;
		}
		for (i = 0; i < ret; i++) {
			counts[((unsigned int)buf[i]) % total]++;
		}
	}
	for (i = 0; i < total; i++) {
		cprintf("%d reads %d\n", i, counts[i]);
	}
	exit(0);
}

int main(void)
{
	int pid, i;
	assert(pipe(__fd) == 0);

	if ((pid = fork()) == 0) {
		fd = __fd[0], close(__fd[1]);
		process_main();
	}
	assert(pid > 0);

	fd = __fd[1], close(__fd[0]);
	memset(tids, 0, sizeof(thread_t) * total);
	for (i = 0; i < total; i++) {
		if (thread(thread_main, (void *)(long)i, tids + i) != 0) {
			goto failed;
		}
	}

	int exit_code;
	for (i = 0; i < total; i++) {
		if (thread_wait(tids + i, &exit_code) != 0 || exit_code != 0) {
			goto failed;
		}
	}

	for (i = 0; i < total; i++) {
		yield();
	}

	close(__fd[0]), close(__fd[1]);
	assert(waitpid(pid, &exit_code) == 0 && exit_code == 0);
	cprintf("pipetest2 pass.\n");
	return 0;

failed:
	for (i = 0; i < total; i++) {
		if (tids[i].pid > 0) {
			kill(tids[i].pid);
		}
	}
	kill(pid);
	panic("FAIL: T.T\n");
}
