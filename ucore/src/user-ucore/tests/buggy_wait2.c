#include <stdio.h>
#include <ulib.h>
#include <unistd.h>

uintptr_t addr = 0;
const size_t size = 4096;

int main(void)
{
	assert(mmap(&addr, size, MMAP_WRITE) == 0);

	int *exit_codep = (int *)addr, pid;

	if ((pid = fork()) == 0) {
		cprintf("child fork ok.\n");
		exit(0);
	}

	assert(waitpid(pid, exit_codep) == 0 && *exit_codep == 0);
	cprintf("buggy_wait2 pass.\n");
	return 0;
}
