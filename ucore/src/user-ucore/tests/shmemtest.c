#include <ulib.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>

void *buf1, *buf2;

int main(void)
{
	assert((buf1 = shmem_malloc(8192)) != NULL);
	assert((buf2 = malloc(4096)) != NULL);
	cprintf("BUF1=0x%08x, BUF2=0x%08x\n", buf1, buf2);

	int i;
	for (i = 0; i < 4096; i++) {
		*(char *)(buf1 + i) = (char)i;
	}
	memset(buf2, 0, 4096);

	int pid, exit_code;
	if ((pid = fork()) == 0) {
		for (i = 0; i < 4096; i++) {
			assert(*(char *)(buf1 + i) == (char)i);
		}
		memcpy(buf1 + 4096, buf1, 4096);
		memset(buf1, 0, 4096);
		memset(buf2, 0xFF, 4096);
		exit(0);
	}
	assert(pid > 0 && waitpid(pid, &exit_code) == 0 && exit_code == 0);

	for (i = 0; i < 4096; i++) {
		assert(*(char *)(buf1 + 4096 + i) == (char)i);
		assert(*(char *)(buf1 + i) == 0);
		assert(*(char *)(buf2 + i) == 0);
	}

	free(buf1);
	free(buf2);

	cprintf("shmemtest pass.\n");
	return 0;
}
