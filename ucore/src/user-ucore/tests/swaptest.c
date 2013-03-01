#include <stdio.h>
#include <ulib.h>
#include <string.h>
#include <malloc.h>

const int size = 5 * 1024 * 1024;
char *buffer;

int pid[10] = { 0 }, pids;

void do_yield(void)
{
	int i;
	for (i = 0; i < 5; i++) {
		yield();
	}
}

void work(int num, bool print)
{
#define printf(...)                             \
    do {                                        \
        if ((print)) {                          \
            cprintf(__VA_ARGS__);               \
        }                                       \
    } while (0)

	do_yield();

	int i, j;
	for (i = 0; i < size; i++) {
		assert(buffer[i] == (char)(i * i));
	}

	printf("check cow ok.\n");

	char c = (char)num;

	for (i = 0; i < 5; i++, c++) {
		printf("round %d\n", i);
		memset(buffer, c, size);
		for (j = 0; j < size; j++) {
			assert(buffer[i] == c);
		}
	}

	do_yield();

	printf("child check ok.\n");

#undef printf
}

int main(void)
{
	assert((buffer = malloc(size)) != NULL);
	cprintf("buffer size = %08x\n", size);

	pids = sizeof(pid) / sizeof(pid[0]);

	int i;
	for (i = 0; i < size; i++) {
		buffer[i] = (char)(i * i);
	}

	for (i = 0; i < pids; i++) {
		if ((pid[i] = fork()) == 0) {
			sleep((pids - i) * 10);
			cprintf("child %d fork ok, pid = %d.\n", i, getpid());
			work(getpid(), i == 0);
			exit(0xbee);
		}
		if (pid[i] < 0) {
			goto failed;
		}
	}
	cprintf("parent init ok.\n");

	for (i = 0; i < pids; i++) {
		int exit_code, ret;
		if ((ret = waitpid(pid[i], &exit_code)) == 0) {
			if (exit_code == 0xbee) {
				continue;
			}
		}
		cprintf("wait failed: %d, pid = %d, ret = %d, exit = %x.\n",
			i, pid[i], ret, exit_code);
		goto failed;
	}

	cprintf("wait ok.\n");
	for (i = 0; i < size; i++) {
		assert(buffer[i] == (char)(i * i));
	}

	cprintf("check buffer ok.\n");
	cprintf("swaptest pass.\n");
	return 0;

failed:
	for (i = 0; i < pids; i++) {
		if (pid[i] > 0) {
			kill(pid[i]);
		}
	}
	panic("FAIL: T.T\n");
}
