#include <stdio.h>
#include <string.h>
#include <mmu.h>
#include <trap.h>
#include <monitor.h>
#include <kdebug.h>

/* *
 * Simple command-line kernel monitor useful for controlling the
 * kernel and exploring the system interactively.
 * */

struct command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func) (int argc, char **argv, struct trapframe * tf);
};

static struct command commands[] = {
	{"help", "Display this list of commands.", mon_help},
	{"kerninfo", "Display information about the kernel.", mon_kerninfo},
};

/* return if kernel is panic, in kern/debug/panic.c */
bool is_kernel_panic(void);

#define NCOMMANDS (sizeof(commands)/sizeof(struct command))

/***** Kernel monitor command interpreter *****/

#define MAXARGS         16
#define WHITESPACE      " \t\n\r"

/* parse - parse the command buffer into whitespace-separated arguments */
static int parse(char *buf, char **argv)
{
	int argc = 0;
	while (1) {
		// find global whitespace
		while (*buf != '\0' && strchr(WHITESPACE, *buf) != NULL) {
			*buf++ = '\0';
		}
		if (*buf == '\0') {
			break;
		}
		// save and scan past next arg
		if (argc == MAXARGS - 1) {
			kprintf("Too many arguments.\n");
		}
		argv[argc++] = buf;
		while (*buf != '\0' && strchr(WHITESPACE, *buf) == NULL) {
			buf++;
		}
	}
	return argc;
}

/* *
 * runcmd - parse the input string, split it into separated arguments
 * and then lookup and invoke some related commands/
 * */
static int runcmd(char *buf, struct trapframe *tf)
{
	char *argv[MAXARGS];
	int argc = parse(buf, argv);
	if (!argc) {
		return 0;
	}
	int i;
	for (i = 0; i < NCOMMANDS; i++) {
		if (!strcmp(commands[i].name, argv[0])) {
			return commands[i].func(argc - 1, argv + 1, tf);
		}
	}
	kprintf("Unknown command '");
	kprintf(argv[0]);
	kprintf("'\n");
	return 0;
}

/***** Implementations of basic kernel monitor commands *****/

void monitor(struct trapframe *tf)
{
	kprintf("Welcome to the kernel debug monitor!!\n");
	kprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL) {
		print_trapframe(tf);
	}

	char *buf;
	while (1) {
		if ((buf = readline("K> ")) != NULL) {
			if (runcmd(buf, tf) < 0) {
				break;
			}
		}
	}
}

/* mon_help - print the information about mon_* functions */
int mon_help(int argc, char **argv, struct trapframe *tf)
{
	int i;
	for (i = 0; i < NCOMMANDS; i++) {
		kprintf(commands[i].name);
		kprintf(" - ");
		kprintf(commands[i].desc);
		kprintf("\n");
	}
	return 0;
}

/* *
 * mon_kerninfo - call print_kerninfo in kern/debug/kdebug.c to
 * print the memory occupancy in kernel.
 * */
int mon_kerninfo(int argc, char **argv, struct trapframe *tf)
{
	print_kerninfo();
	return 0;
}
