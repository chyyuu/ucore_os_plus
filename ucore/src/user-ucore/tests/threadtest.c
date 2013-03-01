#include <ulib.h>
#include <stdio.h>
#include <thread.h>

int test(void *arg)
{
	cprintf("child ok.\n");
	return 0xbee;
}

int main(void)
{
	thread_t tid;
	assert(thread(test, NULL, &tid) == 0);
	cprintf("thread ok.\n");

	int exit_code;
	assert(thread_wait(&tid, &exit_code) == 0 && exit_code == 0xbee);

	cprintf("threadtest pass.\n");
	return 0;
}
