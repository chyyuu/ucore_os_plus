#include <ulib.h>
#include <stdio.h>
#include <string.h>
#include <file.h>

int main(void)
{
	int pid, ret;
	if ((pid = fork()) == 0) {
		do {
			char c;
			ret = read(0, &c, sizeof(c));
			assert(ret == 1);
		} while (1);
	}
	assert(pid > 0);

	sleep(100);
	kill(pid);

	assert(waitpid(pid, &ret) == 0 && ret != 0);
	cprintf("fread_test2 pass.\n");
	return 0;
}
