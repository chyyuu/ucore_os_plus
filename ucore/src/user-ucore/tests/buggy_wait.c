#include <stdio.h>
#include <ulib.h>
#include <unistd.h>
#include <thread.h>

uintptr_t addr = 0;
const size_t size = 4096;

int test(void *arg)
{
	assert(addr != 0 && munmap(addr, size) == 0);
	cprintf("child munmap ok.\n");
	return 0;
}

int main(void)
{
	assert(mmap(&addr, size, MMAP_WRITE) == 0);

	int *exit_codep = (int *)addr;

	thread_t tid;
	assert(thread(test, NULL, &tid) == 0);
	assert(thread_wait(&tid, exit_codep) != 0);

	thread_wait(&tid, NULL);
	cprintf("buggy_wait pass.\n");
	return 0;
}
