#include <stdio.h>
#include <ulib.h>
#include <unistd.h>
#include <thread.h>

uintptr_t addr = 0;
const size_t size = 4096 * 2;

int pid;

int test(void *arg)
{
	assert(addr != 0 && munmap(addr, size) == 0);
	send_event(pid, 1481);
	cprintf("child munmap ok.\n");
	return 0;
}

int main(void)
{
	assert(mmap(&addr, size, MMAP_WRITE) == 0);

	pid = getpid();

	int *pidp = (int *)addr, *eventp = (int *)(addr + 4096);

	thread_t tid;
	assert(thread(test, NULL, &tid) == 0);
	assert(recv_event(pidp, eventp) != 0);

	thread_wait(&tid, NULL);
	cprintf("buggy_event pass.\n");
	return 0;
}
