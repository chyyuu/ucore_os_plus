#include <console.h>

#include <arch.h>
#include <assert.h>
#include <proc.h>

static int tty;
static struct termios default_settings;

static int aio_daemon_pid;
static char buffer[CONSOLE_BUFFER_SIZE];
static int head_ptr, tail_ptr;

/**
 * The daemon who collects input characters and deliver signals to the main process.
 */
static void cons_daemon(void)
{
	while (1) {
		syscall3(__NR_read, tty, (long)&(buffer[head_ptr]), 1);
		head_ptr = (head_ptr + 1) % CONSOLE_BUFFER_SIZE;
		if (pls_read(current)->arch.host == NULL) {
			syscall2(__NR_kill, syscall0(__NR_getppid), SIGIO);
		} else {
			syscall2(__NR_kill,
				 pls_read(current)->arch.host->host_pid, SIGIO);
		}
	}
}

/**
 * open /dev/tty as the console device and adjust the terminal's attributes
 */
void cons_init()
{
	struct termios raw_settings;

	char tty_name[] = "/dev/tty";
	tty = syscall2(__NR_open, (long)tty_name, HOST_O_RDWR);

	if (tty == -1) {
		syscall1(__NR_exit, 1);
	}

	syscall3(__NR_ioctl, tty, TCGETS, (long)&default_settings);

	raw_settings = default_settings;
	raw_settings.c_iflag &= ~(IGNBRK | BRKINT | INLCR | IGNCR | IXON);
	raw_settings.c_lflag |= ISIG;
	raw_settings.c_lflag &= ~(ECHO | ECHONL | ECHOE | ICANON | IEXTEN);
	raw_settings.c_cflag &= ~(CSIZE | PARENB);
	raw_settings.c_cflag |= CS8;
	raw_settings.c_cc[VQUIT] = 034;
	raw_settings.c_cc[VINTR] = 0;
	raw_settings.c_cc[VERASE] = 0;
	raw_settings.c_cc[VWERASE] = 0;
	syscall3(__NR_ioctl, tty, TCSETS, (long)&raw_settings);

	head_ptr = tail_ptr = 0;
	int pid = syscall0(__NR_fork);
	if (pid < 0)
		panic("cannot start aio daemon");
	else if (pid == 0)	/* aio daemon */
		cons_daemon();

	aio_daemon_pid = pid;
}

/**
 * close /dev/tty, reset the terminal's attributes, stop daemon and release shared memory.
 */
void cons_dtor(void)
{
	syscall2(__NR_kill, aio_daemon_pid, SIGKILL);
	syscall3(__NR_ioctl, tty, TCSETS, (long)&default_settings);
	syscall1(__NR_close, tty);
}

/**
 * put a character to the console (tty here)
 * @param c the character to be put
 * TODO: we need tty driver
 */
void cons_putc(int c)
{
	char buf = (char)c;
	syscall3(__NR_write, tty, (long)&buf, 1);
}

/**
 * get a character from the console (tty here)
 * @return the charater got
 */
int cons_getc(void)
{
	if (head_ptr == tail_ptr)
		return 0;

	int c;
	c = (int)buffer[tail_ptr];
	tail_ptr = (tail_ptr + 1) % CONSOLE_BUFFER_SIZE;
	return c;
}

/**
 * !TODO
 */
void serial_intr(void)
{
}

/**
 * !TODO
 */
void kbd_intr(void)
{
}
