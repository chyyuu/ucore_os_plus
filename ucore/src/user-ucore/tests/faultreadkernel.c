#include <stdio.h>
#include <ulib.h>

int main(void)
{
	if (sizeof(uintptr_t) == 4)
		cprintf("I read %08x from 0xfac00000!\n",
			*(unsigned *)0xfac00000);
	else
		cprintf("I read %016llx from 0xffff9f8000000000!\n",
			*(unsigned *)0xffff9f8000000000ULL);
	panic("FAIL: T.T\n");
}
