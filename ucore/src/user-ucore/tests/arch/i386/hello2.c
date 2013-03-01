#include <stdio.h>
#include <ulib.h>

int main(void)
{
	fprintf(1, "Hello world!!.\n");
	fprintf(1, "I am process %d.\n", getpid());
	fprintf(1, "hello2 pass.\n");
	return 0;
}
