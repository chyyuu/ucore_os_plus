#include <stdio.h>
#include <ulib.h>
#include <syscall.h>
#include <error.h>

void brkfreetest(void)
{
	uintptr_t oldbrk = 0;
	assert(sys_brk(&oldbrk) == 0);

	uintptr_t newbrk = oldbrk + 4096;
	assert(sys_brk(&newbrk) == 0 && newbrk >= oldbrk + 4096);

	char *p = (void *)oldbrk;
	int i;
	for (i = 0; i < 4096; i++) {
		p[i] = (char)(i * 31 + (i & 0xF));
	}
	for (i = 0; i < 4096; i++) {
		assert(p[i] == (char)(i * 31 + (i & 0xF)));
	}
	newbrk = oldbrk;
	assert(sys_brk(&newbrk) == 0 && newbrk == oldbrk);

	cprintf("page fault!!\n");
	p[0] = 0;
}

int main(void)
{
	int pid, exit_code;
	if ((pid = fork()) == 0) {
		brkfreetest();
		exit(0xdead);
	}
	assert(pid > 0);
	assert(waitpid(pid, &exit_code) == 0 && exit_code != 0xdead);
	assert(exit_code != -E_PANIC);

	cprintf("brkfreetest pass.\n");
	return 0;
}
