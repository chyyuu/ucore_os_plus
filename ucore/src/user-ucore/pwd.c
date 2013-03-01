#include <ulib.h>
#include <stdio.h>
#include <dir.h>

#define printf(...)                     fprintf(1, __VA_ARGS__)
#define BUFSIZE                         4096

int main(int argc, char **argv)
{
	int ret;
	static char cwdbuf[BUFSIZE];
	if ((ret = getcwd(cwdbuf, sizeof(cwdbuf))) != 0) {
		return ret;
	}
	printf("%s\n", cwdbuf);
	return 0;
}
