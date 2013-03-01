#include <ulib.h>
#include <stdio.h>
#include <string.h>
#include <stat.h>
#include <file.h>

static void testfd(const char *name, int fd)
{
	struct stat __stat, *stat = &__stat;
	int ret = fstat(fd, stat);
	assert(ret == 0);
	print_stat(name, fd, stat);
}

int main(void)
{
	int fd = dup(1);
	assert(fd >= 0);

	testfd("stdin", 0);
	testfd("stdout", 1);
	testfd("dup: stdout", fd);

	int size = 1024, len = 0;
	char buf[size];
	len += snprintf(buf + len, size - len, "Hello world!!.\n");
	len += snprintf(buf + len, size - len, "I am process %d.\n", getpid());
	write(fd, buf, len);

	int ret;
	while ((ret = dup(fd)) >= 0)
		/* do nothing */ ;

	close(fd);
	len = snprintf(buf, size, "FAIL: T.T\n");
	write(fd, buf, len);
	cprintf("dup fd ok.\n");

	int pid;
	if ((pid = fork()) == 0) {
		sleep(10);
		len = snprintf(buf, size, "fork fd ok.\n");
		ret = write(1, buf, len);
		assert(ret == len);
		exit(0);
	}
	assert(pid > 0);
	assert(waitpid(pid, &ret) == 0 && ret == 0);
	cprintf("fwrite_test pass.\n");
	return 0;
}
