#include <stdio.h>
#include <ulib.h>
#include <unistd.h>

int main(void)
{
	const int size = 4096;
	void *mapped[10] = { 0 };

	uintptr_t addr = 0;

	assert(mmap(NULL, size, 0) != 0
	       && mmap((void *)0xC0000000, size, 0) != 0);

	int i;
	for (i = 0; i < 10; i++) {
		assert(mmap(&addr, size, MMAP_WRITE) == 0 && addr != 0);
		mapped[i] = (void *)addr, addr = 0;
	}
	cprintf("mmap step1 ok.\n");

	for (i = 0; i < 10; i++) {
		assert(munmap((uintptr_t) mapped[i], size) == 0);
	}

	cprintf("munmap step1 ok.\n");

	addr = 0x40000000;
	assert(mmap(&addr, size, MMAP_WRITE) == 0);
	mapped[0] = (void *)addr;

	addr = 0x40001000;
	assert(mmap(&addr, size, MMAP_WRITE) == 0);
	mapped[1] = (void *)addr;

	cprintf("mmap step2 ok.\n");

	addr = 0x40001800;
	assert(mmap(&addr, 100, 0) != 0);

	cprintf("mmap step3 ok.\n");

	assert(munmap((uintptr_t) mapped[0], size * 2 + 100) == 0);

	cprintf("mumap step2 ok.\n");

	addr = 0;
	assert(mmap(&addr, 128, MMAP_WRITE) == 0 && addr != 0);
	mapped[0] = (void *)addr;

	char *buffer = mapped[0];
	for (i = 0; i < 128; i++) {
		buffer[i] = (char)(i * i);
	}

	for (i = 0; i < 128; i++) {
		assert(buffer[i] == (char)(i * i));
	}

	cprintf("mmaptest pass.\n");
	return 0;
}
