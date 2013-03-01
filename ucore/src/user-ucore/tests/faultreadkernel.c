#include <stdio.h>
#include <ulib.h>

int main(void)
{
	cprintf("I read %016llx from 0xffff9f8000000000!\n",
		*(unsigned *)0xffff9f8000000000);
	panic("FAIL: T.T\n");
}
