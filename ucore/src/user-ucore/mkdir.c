#include <ulib.h>
#include <stdio.h>
#include <dir.h>

#define printf(...)                     fprintf(1, __VA_ARGS__)

void usage(void)
{
	printf("usage: mkdir dir [...]\n");
}

int main(int argc, char **argv)
{
	if (argc == 1) {
		usage();
		return -1;
	} else {
		int i, ret;
		for (i = 1; i < argc; i++) {
			if ((ret = mkdir(argv[i])) != 0) {
				return ret;
			}
		}
	}
	return 0;
}
