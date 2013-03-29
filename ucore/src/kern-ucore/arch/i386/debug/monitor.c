#include <arch.h>
#include <stdio.h>
#include <string.h>
#include <mmu.h>
#include <trap.h>
#include <monitor.h>
#include <kdebug.h>
#include <kio.h>

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
	{"backtrace", "Print backtrace of stack frame.", mon_backtrace},
	{"continue", "Continue execution.", mon_continue},
	{"step", "Run single instruction.", mon_step},
	{"break", "Set breakpoint at specified linear address.\n"
	 "    '-rx': the specified debug register(0~3)\n"
	 "    @example: breakpoint -r0 0xC0100000", mon_breakpoint},
	{"watch", "Set watchpoint at specified linear address.\n"
	 "    '-rx': the specified debug register(0~3)\n"
	 "    '-tx': -tw - break on data writes only\n"
	 "           -ta - break on data reads or writes\n"
	 "    '-lx': -l1 - 1-byte length\n"
	 "           -l2 - 2-byte length\n"
	 "           -l4 - 4-byte length\n"
	 "    default is -tw -l4\n"
	 "    @example: watchpoint -r3 0xC020000", mon_watchpoint},
	{"deldr", "Delete a breakpoint or watchpoint.\n"
	 "    'x': the specified debug register(0~3)\n"
	 "    @example: delbp 3", mon_delete_dr},
	{"listdr", "List all breakpoints or watchpoints.", mon_list_dr},
	{"halt", "shutdown qemu(modified)",mon_halt},
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
			kprintf("Too many arguments (max %d).\n", MAXARGS);
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
	if (argc == 0) {
		return 0;
	}
	int i;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(commands[i].name, argv[0]) == 0) {
			return commands[i].func(argc - 1, argv + 1, tf);
		}
	}
	kprintf("Unknown command '%s'\n", argv[0]);
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
		kprintf("%s - %s\n", commands[i].name, commands[i].desc);
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

/* *
 * mon_backtrace - call print_stackframe in kern/debug/kdebug.c to
 * print a backtrace of the stack.
 * */
int mon_backtrace(int argc, char **argv, struct trapframe *tf)
{
	print_stackframe();
	return 0;
}

/* mon_continue - continue execution if it isn't kernel panic */
int mon_continue(int argc, char **argv, struct trapframe *tf)
{
	if (is_kernel_panic()) {
		kprintf("can't continue execution in kernel panic.\n");
		return 0;
	}
	if (tf != NULL) {
		tf->tf_eflags &= ~FL_TF;
	}
	return -1;
}

/* mon_step - run a single step */
int mon_step(int argc, char **argv, struct trapframe *tf)
{
	if (is_kernel_panic()) {
		kprintf("can't continue execution in kernel panic.\n");
		return 0;
	}
	if (tf != NULL) {
		tf->tf_eflags |= FL_TF;
		return -1;
	}
	kprintf("trapframe is NULL, can't run step.\n");
	return 0;
}

/* mon_breakpoint - set a breakpoint */
int mon_breakpoint(int argc, char **argv, struct trapframe *tf)
{
	if (argc != 2) {
		kprintf("needs 2 parameter(s).\n");
		return 0;
	}
	uintptr_t addr;
	unsigned regnum = MAX_DR_NUM, type = 0, len = 3;
	int i;
	for (i = 0; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] != 'r' || strlen(argv[i]) != 3) {
				goto bad_argv;
			} else {
				switch (argv[i][2]) {
				case '0' ... '3':
					regnum = argv[i][2] - '0';
					break;
				default:
					goto bad_argv;
				}
			}
		} else {
			char *endptr;
			addr = strtol(argv[i], &endptr, 16);
			if (*endptr != '\0') {
				goto bad_argv;
			}
		}
	}
	int ret = debug_enable_dr(regnum, addr, type, len);
	kprintf("set breakpoint [%d] at 0x%08x: %s.\n", regnum, addr,
		(ret == 0) ? "successed" : "failed");
	return 0;

bad_argv:
	kprintf("unknow parameter(s): [%d] %s\n", i, argv[i]);
	return 0;
}

/* mon_watchpoint - set a watchpoint */
int mon_watchpoint(int argc, char **argv, struct trapframe *tf)
{
	if (argc < 2) {
		kprintf("needs at least 2 parameter(s).\n");
		return 0;
	}
	uintptr_t addr;
	unsigned regnum = MAX_DR_NUM, type = 1, len = 3;
	int i;
	for (i = 0; i < argc; i++) {
		if (argv[i][0] == '-') {
			char c = argv[i][1];
			if ((c != 'r' && c != 't' && c != 'l')
			    || strlen(argv[i]) != 3) {
				goto bad_argv;
			}
			switch (c) {
			case 'r':
				switch (argv[i][2]) {
				case '0' ... '3':
					regnum = argv[i][2] - '0';
					break;
				default:
					goto bad_argv;
				}
				break;
			case 't':
				switch (argv[i][2]) {
				case 'w':
					type = 1;
					break;
				case 'a':
					type = 3;
					break;
				default:
					goto bad_argv;
				}
				break;
			case 'l':
				switch (argv[i][2]) {
				case '1':
					len = 0;
					break;
				case '2':
					len = 1;
					break;
				case '4':
					len = 3;
					break;
				default:
					goto bad_argv;
				}
				break;
			}
		} else {
			char *endptr;
			addr = strtol(argv[i], &endptr, 16);
			if (*endptr != '\0') {
				goto bad_argv;
			}
		}
	}
	int ret = debug_enable_dr(regnum, addr, type, len);
	kprintf("set watchpoint [%d] at 0x%08x: %s.\n", regnum, addr,
		(ret == 0) ? "successed" : "failed");
	return 0;

bad_argv:
	kprintf("unknow parameter(s): [%d] %s\n", i, argv[i]);
	return 0;
}

/* mon_delete_dr - delete a breakpoint or watchpoint */
int mon_delete_dr(int argc, char **argv, struct trapframe *tf)
{
	if (argc != 1) {
		kprintf("needs 1 parameter(s).\n");
		return 0;
	}
	unsigned regnum = MAX_DR_NUM;
	if (strlen(argv[0]) != 1) {
		goto bad_argv;
	} else {
		switch (argv[0][0]) {
		case '0' ... '3':
			regnum = argv[0][0] - '0';
			break;
		default:
			goto bad_argv;
		}
	}
	int ret = debug_disable_dr(regnum);
	kprintf("delete [%d]: %s.\n", regnum,
		(ret == 0) ? "successed" : "failed");
	return 0;

bad_argv:
	kprintf("unknow parameter(s): [%d] %s\n", 0, argv[0]);
	return 0;
}

/* mon_list_dr - list all debug registers */
int mon_list_dr(int argc, char **argv, struct trapframe *tf)
{
	debug_list_dr();
	return 0;
}

/* *
 * mon_halt - write  io port 0x8900 "Shutdown" to shutdown qemu
 * NOTICE: only used in modified qemu (with 0x8900 IO write support)
 *         in qemu/hw/pc.c::bochs_bios_write function
 * */
int mon_halt(int argc, char **argv, struct trapframe *tf)
{
	char *p = "Shutdown";
	for( ; *p; p++)
		outb(0x8900, *p);

	return 0;
}
