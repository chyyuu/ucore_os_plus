#include <stdio.h>
#include <ulib.h>
#include <string.h>
#include <spipe.h>
#include <malloc.h>

int main(void)
{
	spipe_t pipe;
	assert(spipe(&pipe) == 0);

	int i, pid;
	if ((pid = fork()) == 0) {
		for (i = 0; i < 10; i++) {
			yield();
		}
		if (spipewrite(&pipe, "A", 1) == 1) {
			cprintf("child write ok\n");
		}
		spipeclose(&pipe);
		exit(0);
	}
	assert(pid > 0);

	sleep(100);
	int len;
	char buf[100];
	assert((len = spiperead(&pipe, buf, sizeof(buf))) == 1
	       && buf[0] == 'A');
	assert(spipeclose(&pipe) == 0);
	cprintf("parent read ok\n");
	cprintf("spipetest pass\n");
	return 0;
}
