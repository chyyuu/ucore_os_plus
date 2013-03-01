#include <ulib.h>
#include <stdio.h>
#include <string.h>
#include <file.h>

int main(void)
{
	/* type '^C' to stop reading */
	char c;
	cprintf("now reading...\n");
	do {
		int ret = read(0, &c, sizeof(c));
		assert(ret == 1);
		cprintf("type [%03d] %c.\n", c, c);
	} while (c != 3);
	return 0;
}
