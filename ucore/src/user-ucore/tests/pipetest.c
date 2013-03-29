#include <stdio.h>
#include <ulib.h>
#include <string.h>
#include <file.h>
#include <stat.h>
#include <unistd.h>
#include <malloc.h>

static char buf[4096];

static void test1(void)
{
	int __fd[2];
	assert(pipe(NULL) != 0 && pipe(__fd) == 0);

	int i, pid, fd, len;
	if ((pid = fork()) == 0) {
		fd = __fd[1];
		for (i = 0; i < 10; i++) {
			yield();
		}
		if (write(fd, "A", 1) == 1) {
			cprintf("child write ok\n");
		}
		exit(0);
	}
	assert(pid > 0);

	fd = __fd[0], close(__fd[1]);
	assert((len = read(fd, buf, sizeof(buf))) == 1 && buf[0] == 'A');
	assert(wait() == 0 && close(fd) == 0);

	cprintf("parent read ok\n");
	cprintf("pipetest step1 pass.\n");
}

static void test2(void)
{
	int __fd[2], fd, ret;
	assert(pipe(NULL) != 0 && pipe(__fd) == 0);

	struct stat __stat, *stat = &__stat;

	fd = __fd[0], ret = fstat(fd, stat);
	assert(ret == 0);
	print_stat("pipe0", fd, stat);
	assert(S_ISCHR(stat->st_mode));

	fd = __fd[1], ret = fstat(fd, stat);
	assert(ret == 0);
	print_stat("pipe1", fd, stat);
	assert(S_ISCHR(stat->st_mode));

	close(__fd[0]), close(__fd[1]);
	cprintf("pipetest step2 pass.\n");
}

static void test3(void)
{
	int __fd[2];
	assert(pipe(NULL) != 0 && pipe(__fd) == 0);

	char *msg = "Hello world!!.";
	size_t len = strlen(msg);

	int pid, fd, ret;
	if ((pid = fork()) == 0) {
		fd = __fd[1];
		assert(read(fd, buf, sizeof(buf)) < 0);
		ret = write(fd, msg, len);
		assert(ret == len);
		exit(0);
	}
	assert(pid > 0);

	fd = __fd[0], close(__fd[1]);
	assert(write(fd, msg, len) < 0);

	int total = 0;
	while ((ret = read(fd, buf + total, sizeof(buf) - total)) > 0) {
		total += ret;
	}
	buf[total] = '\0';
	cprintf("pipetest step3:: total %d, len %d, buf %s, msg %s, buf[total-1] %c, buf[total-2] %c\n", total, len, buf,msg,buf[total-1],buf[total-2]);
	assert(total == len && strcmp(buf, msg) == 0);
	assert(wait() == 0 && close(fd) == 0);
	cprintf("pipetest step3 pass.\n");
}

static void test4(void)
{
	const char *name = "test";
	int fd, __fd;
	fd = __fd = mkfifo(name, O_CREAT | O_RDONLY);
	assert(fd >= 0);

	fd = mkfifo(name, O_CREAT | O_RDONLY);
	assert(fd >= 0 && close(fd) == 0);

	assert(mkfifo(name, O_CREAT | O_EXCL | O_RDONLY) < 0);
	assert(mkfifo(name, O_WRONLY) < 0);

	fd = mkfifo(name, O_CREAT | O_WRONLY | O_EXCL);
	assert(fd >= 0 && close(fd) == 0);
	assert(read(__fd, buf, sizeof(buf)) == 0 && close(__fd) == 0);

	fd = mkfifo(name, O_CREAT | O_RDONLY | O_EXCL);
	assert(fd >= 0);

	int pid, ret;
	if ((pid = fork()) == 0) {
		fd = mkfifo(name, O_CREAT | O_WRONLY | O_EXCL);
		assert(fd >= 0);
		memset(buf, 'A', sizeof(buf));
		int ret = write(fd, buf, sizeof(buf));
		assert(ret == sizeof(buf));
		exit(0);
	}
	assert(pid > 0);

	size_t total = 0;
	while ((ret = read(fd, buf + total, sizeof(buf) - total)) > 0) {
		total += ret;
	}
	assert(total == sizeof(buf));

	int i;
	for (i = 0; i < total; i++) {
		assert(buf[i] == 'A');
	}
	cprintf("pipetest step4 pass.\n");
}

int main(void)
{
	test1();
	test2();
	test3();
	test4();
	cprintf("pipetest pass.\n");
	return 0;
}
