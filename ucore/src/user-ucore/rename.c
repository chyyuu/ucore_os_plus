#include <ulib.h>
#include <stdio.h>
#include <dir.h>

#define printf(...)                     fprintf(1, __VA_ARGS__)

void usage(void)
{
	printf("usage: rename from to\n");
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		usage();
		return -1;
	}
	return rename(argv[1], argv[2]);
}
