#include <ulib.h>
#include <stdio.h>
#include <string.h>
#include <file.h>

#define printf(...)                 fprintf(1, __VA_ARGS__)

int main(void)
{
	int fd[2], ret = pipe(fd);
	assert(ret == 0);

	int pid;
	if ((pid = fork()) == 0) {
		close(0), close(fd[1]);
		if ((ret = dup2(fd[0], 0)) < 0) {
			exit(ret);
		}
		ret = exec("/testbin/robot");
		panic("child exec failed: %e\n", ret);
	}
	assert(pid > 0);

	close(fd[0]);

	int i;
	for (i = 0; i < 32; i++) {
		fprintf(fd[1], "%02d: Hello world!!.\n", i);
	}
	close(fd[1]);

	assert(wait() == 0);
	cprintf("sfs_exectest1 pass.\n");
	return 0;
}
