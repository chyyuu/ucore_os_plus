#include <stdio.h>
#include <ulib.h>
#include <string.h>
#include <file.h>
#include <unistd.h>

int main(void)
{
	int fd = 0, ret;
	char buf[5];
	buf[0] = buf[1] = buf[2] = buf[3] = 'a';
	buf[4] = '\n';
	fd = open("null:", O_RDWR);
	cprintf("NULL fd is %d\n", fd);
	ret = write(fd, "hello", 5);
	cprintf("write %d to NULL\n", ret);
	ret = read(fd, buf, 5);
	cprintf("read %d from NULL, buf is %s\n", ret, buf);

	fprintf(1, "Hello world!!.\n");
	fprintf(1, "I am process %d.\n", getpid());
	fprintf(1, "hello2 pass.\n");
	return 0;
}
