#include <ulib.h>
#include <unistd.h>
#include <file.h>
#include <stat.h>

int main(int argc, char **argv);

static int initfd(int fd2, const char *path, uint32_t open_flags)
{
	struct stat __stat, *stat = &__stat;
	int ret, fd1;
	if ((ret = fstat(fd2, stat)) != 0) {
		if ((fd1 = open(path, open_flags)) < 0 || fd1 == fd2) {
			return fd1;
		}
		close(fd2);
		ret = dup2(fd1, fd2);
		close(fd1);
	}
	return ret;
}

void umain(int argc, char **argv)
{
	int fd;
	if ((fd = initfd(0, "stdin:", O_RDONLY)) < 0) {
		warn("open <stdin> failed: %e.\n", fd);
	}
	if ((fd = initfd(1, "stdout:", O_WRONLY)) < 0) {
		warn("open <stdout> failed: %e.\n", fd);
	}
	if ((fd = initfd(2, "stdout:", O_WRONLY)) < 0) {
		warn("open <stderr> failed: %e.\n", fd);
	}
	int ret = main(argc, argv);
	exit(ret);
}
