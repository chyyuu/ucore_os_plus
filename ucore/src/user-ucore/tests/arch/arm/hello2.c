#include <stdio.h>
#include <ulib.h>

#define printf(...)                     fprintf(1, __VA_ARGS__)

#define perror cprintf
#define pid_t int
#define FORK_CNT 1

int __fork()
{
	int res = -1;
	asm volatile ("mov r7, #2;"
		      "swi #0;" "mov %0,r0;":"=r" (res)::"r0", "r7");
	return res;
}

void test()
{
	int i = 0;
	pid_t pid = -1;
	pid_t pids[FORK_CNT];
	for (i = 0; i < FORK_CNT; i++) {
		pid = __fork();
		if (pid < 0) {
			perror("fork()");
			return;
		}
		if (pid == 0) {
			printf("I'm child\n");
			exit(1);
		} else {
			pids[i] = pid;
			printf("Fork Child %d\n", pid);
		}
	}
	int status = 0;
#if 1
	for (i = 0; i < FORK_CNT; i++) {
		//waitpid(pids[i], &status, 0);
		wait();
		printf("Child %d end %d\n", pids[i], status);
	}
#else
	while (1)
		sched_yield();
#endif
	printf("OK...\n");
}

int main(void)
{
	fprintf(1, "Hello world!!.\n");
	fprintf(1, "I am process %d.\n", getpid());
	fprintf(1, "hello2 pass.\n");
	test();
	return 0;
}
