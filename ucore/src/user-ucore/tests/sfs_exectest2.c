#include <ulib.h>
#include <stdio.h>
#include <string.h>
#include <file.h>

#define printf(...)                 fprintf(1, __VA_ARGS__)

const char *cmd = "/testbin/sfs_exectest2";

int main(int argc, char **argv)
{
	int i;
	for (i = 0; i < argc; i++) {
		printf("%d-%d: %s\n", argc, i, argv[i]);
	}
	switch (argc) {
	case 1:
		exec(cmd, "arg0");
	case 2:
		exec(cmd, "arg0", "arg1");
	case 3:
		exec(cmd, "arg0", "arg1", "arg2");
	case 4:
		exec(cmd, "arg0", "arg1", "arg2", "arg3");
	case 5:
		exec(cmd, "arg0", "arg1", "arg2", "arg3", "arg4");
	}
	cprintf("sfs_exectest2 pass.\n");
	return 0;
}
