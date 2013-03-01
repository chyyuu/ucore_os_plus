/*
 * =====================================================================================
 *
 *       Filename:  kgdb-stub.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/31/2012 12:43:46 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Chen Yuheng (Chen Yuheng), chyh1990@163.com
 *   Organization:  Tsinghua Unv.
 *
 * =====================================================================================
 */

#include <board.h>
#include <memlayout.h>
#include <types.h>
#include <arm.h>
#include <stdio.h>
#include <trap.h>
#include <kio.h>
#include <kdebug.h>
#include <string.h>
#include <assert.h>
#include <kgdb-stub.h>

#define MAX_KGDB_BP      16

#define SBPT_AVAIL (1<<0)
#define SBPT_ACTIVE (1<<1)

//#define KGDB_TRACE

#ifdef KGDB_TRACE
#define kgdb_trace(...) kprintf(__VA_ARGS__)
#else
#define kgdb_trace(...)
#endif

struct soft_bpt {
	uint32_t addr;		/* address breapoint is present at */
	uint32_t inst;		/* replaced instruction */
	uint32_t flags;
};				// __attribute__((packed));

struct soft_bpt breakpoints[MAX_KGDB_BP] = { {0} };

static const char hexchars[] = "0123456789abcdef";

/* CPU 400MHz? */
static inline void platform_1u_delay()
{
	int i;
	for (i = 0; i < 100; i++)
		asm volatile ("nop");
}

static inline void platform_udelay(int delay)
{
	for (; delay >= 0; delay--) {
		platform_1u_delay();
	}
}

static int hex(char ch)
{
	if ((ch >= 'a') && (ch <= 'f'))
		return (ch - 'a' + 10);
	if ((ch >= '0') && (ch <= '9'))
		return (ch - '0');
	if ((ch >= 'A') && (ch <= 'F'))
		return (ch - 'A' + 10);
	return (-1);
}

/* convert the memory pointed to by mem into hex, placing result in buf */
/* return a pointer to the last char put in buf (null) */
static char *mem2hex(char *mem, char *buf, int count)
{
	int i;
	unsigned char ch;

	for (i = 0; i < count; i++) {
		ch = *mem++;
		*buf++ = hexchars[ch >> 4];
		*buf++ = hexchars[ch % 16];
	}
	*buf = 0;
	return (buf);
}

/* convert the hex array pointed to by buf into binary to be placed in mem */
/* return a pointer to the character AFTER the last byte written */
static char *hex2mem(char *buf, char *mem, int count)
{
	int i;
	unsigned char ch;

	for (i = 0; i < count; i++) {
		ch = hex(*buf++) << 4;
		ch = ch + hex(*buf++);
		*mem++ = ch;
	}
	return (mem);
}

/*
 * WHILE WE FIND NICE HEX CHARS, BUILD AN INT
 * RETURN NUMBER OF CHARS PROCESSED
 */
int hexToInt(char **ptr, int *intValue)
{
	int numChars = 0;
	int hexValue;

	*intValue = 0;

	while (**ptr) {
		hexValue = hex(**ptr);
		if (hexValue >= 0) {
			*intValue = (*intValue << 4) | hexValue;
			numChars++;
		} else
			break;

		(*ptr)++;
	}

	return (numChars);
}

static struct soft_bpt *find_bp(uint32_t addr)
{
	int i = 0;
	for (; i < MAX_KGDB_BP; i++) {
		if (breakpoints[i].addr == addr
		    && (breakpoints[i].flags & SBPT_AVAIL))
			return &breakpoints[i];
	}
	return NULL;
}

static void activate_bps()
{
	int i = 0;
	for (; i < MAX_KGDB_BP; i++) {
		if ((breakpoints[i].flags & SBPT_AVAIL)
		    && !(breakpoints[i].flags & SBPT_ACTIVE)
		    ) {
			if (kdebug_check_mem_range(breakpoints[i].addr, 4))
				continue;
			breakpoints[i].inst = *(uint32_t *) breakpoints[i].addr;
			breakpoints[i].flags |= SBPT_ACTIVE;
			*(uint32_t *) breakpoints[i].addr = KGDB_BP_INSTR;
		}
	}

}

static void deactivate_bps()
{
	int i = 0;
	for (; i < MAX_KGDB_BP; i++) {
		if ((breakpoints[i].flags & SBPT_AVAIL)
		    && (breakpoints[i].flags & SBPT_ACTIVE)
		    ) {
			*(uint32_t *) breakpoints[i].addr = breakpoints[i].inst;
			breakpoints[i].flags &= ~SBPT_ACTIVE;
		}
	}

}

static struct soft_bpt *find_free_bp()
{
	int i;
	for (i = 0; i < MAX_KGDB_BP; i++) {
		if (!(breakpoints[i].flags & SBPT_AVAIL))
			return &breakpoints[i];
	}
	return NULL;
}

int remove_bp(uint32_t addr)
{
	struct soft_bpt *bp = find_bp(addr);
	if (!bp)
		return -1;
	if (kdebug_check_mem_range(addr, 4))
		return -1;
	/* restore */
	*(uint32_t *) (bp->addr) = bp->inst;
	bp->flags = 0;
	return 0;
}

static struct soft_bpt *install_bp(uint32_t addr)
{
	struct soft_bpt *bp = find_free_bp();
	if (find_bp(addr))
		return NULL;
	if (!bp)
		return NULL;
	bp->addr = addr;
	if (bp->inst == KGDB_BP_INSTR)
		return NULL;
	if (kdebug_check_mem_range(addr, 4))
		return NULL;
	bp->flags = SBPT_AVAIL;
	return bp;
}

int setup_bp(uint32_t addr)
{
	return (install_bp(addr) == NULL) ? -1 : 0;
}

void kgdb_init()
{
	int i;
	for (i = 0; i < MAX_KGDB_BP; i++)
		breakpoints[i].flags = 0;
}

static char kgdb_buffer[512];

#ifdef HAS_SDS
#include <intel_sds.h>
#define __kgdb_put_char(c) sds_putc(1, c)
#define __kgdb_get_char() sds_proc_data(1)
#define __kgdb_flush_char() while(!sds_poll_proc());
#define __kgdb_poll_char() (sds_poll_proc())
#else
#define __kgdb_put_char(c)
#define __kgdb_get_char() ('#')
#define __kgdb_flush_char()
#define __kgdb_poll_char() (0)
#endif

static void kgdb_drain_input()
{
	while (__kgdb_get_char() != -1) ;
}

static int kgdb_get_packet(char *data)
{
	int cnt = 0;
	int c;
	while (1) {
		c = __kgdb_get_char();
		if (c == -1)
			__kgdb_poll_char();
		else if (c == '$')
			break;
	}
	data[cnt++] = '$';
	while (cnt < 500) {
		c = __kgdb_get_char();
		if (c == -1)
			__kgdb_poll_char();
		else
			data[cnt++] = c;
		if (c == '#')
			break;
	}
	data[cnt] = 0;
	if (cnt > 0 && data[cnt - 1] == '#') {
		return 0;
	} else {
		return -1;
	}
}

/* add $# automatically */
static int kgdb_put_packet(char *data)
{
	int len = strlen(data);
	int i;
	__kgdb_put_char('$');
	for (i = 0; i < len; i++)
		__kgdb_put_char(data[i]);
	__kgdb_put_char('#');
	__kgdb_flush_char();
	return 0;
}

static uint32_t kgdb_get_lr(struct trapframe *tf)
{
	uint32_t mode = tf->tf_sr & 0x1F;
	uint32_t cpsr = read_psrflags();
	uint32_t nc = cpsr & (~0x1F);
	uint32_t lr = 0;
	switch (mode) {
	case ARM_SR_MODE_ABT:
	case ARM_SR_MODE_FIQ:
	case ARM_SR_MODE_IRQ:
	case ARM_SR_MODE_SVC:
	case ARM_SR_MODE_SYS:
	case ARM_SR_MODE_UND:
		nc |= mode;
		write_psrflags(nc);
		asm volatile ("mov %0, lr":"=r" (lr));
		write_psrflags(cpsr);
		return lr;
	case ARM_SR_MODE_USR:
		/* TODO */
	default:
		return 0;
	}
	return 0;
}

static void kgdb_set_lr(struct trapframe *tf, uint32_t lr)
{
	uint32_t mode = tf->tf_sr & 0x1F;
	uint32_t cpsr = read_psrflags();
	uint32_t nc = cpsr & (~0x1F);
	switch (mode) {
	case ARM_SR_MODE_ABT:
	case ARM_SR_MODE_FIQ:
	case ARM_SR_MODE_IRQ:
	case ARM_SR_MODE_SVC:
	case ARM_SR_MODE_SYS:
	case ARM_SR_MODE_UND:
		nc |= mode;
		write_psrflags(nc);
		asm volatile ("mov lr, %0"::"r" (lr));
		write_psrflags(cpsr);
		break;
	case ARM_SR_MODE_USR:
		/* TODO */
	default:
		return;
	}
}

static uint32_t gdb_regs[GDB_MAX_REGS];
static void kgdb_reg2data(struct trapframe *tf, char *buf)
{
	memset(gdb_regs, 0, sizeof(uint32_t) * GDB_MAX_REGS);
	memcpy(gdb_regs, &tf->tf_regs, sizeof(struct pushregs));
	gdb_regs[13] = tf->tf_esp;
	gdb_regs[14] = kgdb_get_lr(tf);
	gdb_regs[15] = tf->tf_epc - 4;
	gdb_regs[_CPSR] = tf->tf_sr;
	//strcpy(buf, "1234abcdcccccccc");
	mem2hex((char *)gdb_regs, buf, sizeof(uint32_t) * GDB_MAX_REGS);

}

static int kgdb_data2reg(struct trapframe *tf, char *buf)
{
	int len = 0;
	char *ptr = buf;
	while (*ptr) {
		if (*ptr == '#')
			break;
		ptr++;
	}
	len = ptr - buf;
	if (len % 8) {
		kgdb_trace("invalid param\n");
		return -1;
	}
	len = len / 2;
	memset(gdb_regs, 0, sizeof(uint32_t) * GDB_MAX_REGS);
	hex2mem(buf, (char *)gdb_regs, len);
	len /= 4;
	memcpy(&tf->tf_regs, gdb_regs, len * sizeof(uint32_t));
	/* TODO set pc, sp, lr, sr */
	return 0;
}

#define _IS_SEP(x) ( (x) == ',' || (x)=='#' || (x)==':')
static int kgdb_parse(const char *buf, uint32_t arg[], int maxlen)
{
	const char *ptr = buf;
	int cnt = 0;
	while (*ptr) {
		if (_IS_SEP(*ptr)) {
			//*ptr = 0;
			arg[cnt++] = kgdb_atoi16(buf);
			buf = ptr + 1;
			if (cnt >= maxlen)
				goto done;
		}
		ptr++;
	}
done:
#ifdef KGDB_TRACE
	kgdb_trace("arg: ");
	int i;
	for (i = 0; i < cnt; i++)
		kgdb_trace("%08x ", arg[i]);
	kgdb_trace("\n");
#endif
	return cnt;
}

static int kgdb_sendmem(char *buf, uint32_t start, uint32_t size)
{
	if (kdebug_check_mem_range(start, size))
		return -1;
	mem2hex((char *)start, buf, size);
	return kgdb_put_packet(kgdb_buffer);
}

static int kgdb_changemem(char *buf, uint32_t start, uint32_t size)
{
	if (kdebug_check_mem_range(start, size)) {
		kgdb_trace("CM\n");
		return -1;
	}
	char *ptr = NULL;
	/* find data start */
	while (*buf) {
		if (*buf == ':') {
			ptr = buf;
			break;
		}
		buf++;
	}
	if (!ptr) {
		kgdb_trace("PT\n");
		return -2;
	}
	ptr++;
	hex2mem(ptr, (char *)start, size);
	return kgdb_put_packet("OK");
}

static int trap_reentry = 0;
#define KGDB_SIGTRAP_REPLY "S05"
#define KGDB_SIGINT_REPLY "S02"

int kgdb_trap(struct trapframe *tf)
{
	int rval = -1;
	uint32_t arg[5];
	if (trap_reentry)
		panic("kgdb_trap reentry\n");
	trap_reentry = 1;
	uint32_t pc = tf->tf_epc - 4;
	//uint32_t inst = *(uint32_t *)(pc);
	struct soft_bpt *bp = find_bp(pc);
	int compilation_bp = (bp == NULL);

	deactivate_bps();

	start_debug();

	kprintf("BP hit: compile=%d, pc=0x%08x...\n", compilation_bp, pc);
#if 0
	rval = kgdb_parse("6a1bbb,2#", arg, 5);
	kprintf("## %d %08x %08x %08x\n", rval, arg[0], arg[1], arg[2]);
	rval = kgdb_parse("6a1bbb,29:a340#", arg, 5);
	kprintf("## %d %08x %08x %08x\n", rval, arg[0], arg[1], arg[2]);
#endif

	/* tell host we are ready */
	if (compilation_bp)
		rval = kgdb_put_packet(KGDB_SIGINT_REPLY);
	else
		rval = kgdb_put_packet(KGDB_SIGTRAP_REPLY);

	kgdb_drain_input();

	while (1) {
		//platform_udelay(200); 
		kgdb_trace("D>");
		rval = kgdb_get_packet(kgdb_buffer);
		if (rval) {
			kprintf("kgdb_trap: fail to get command from host\n");
			goto cont_kernel;
		}
		kgdb_trace("%s\n", kgdb_buffer);
		switch (kgdb_buffer[1]) {
			/* reg */
		case 'g':
			kgdb_reg2data(tf, kgdb_buffer);
			rval = kgdb_put_packet(kgdb_buffer);
			break;
		case 'G':
			rval = kgdb_data2reg(tf, kgdb_buffer + 2);
			if (rval)
				rval = kgdb_put_packet("E14");
			else
				rval = kgdb_put_packet("OK");
			break;
			/* clear bp */
		case 'z':
			/* add bp */
		case 'Z':
			rval = kgdb_parse(kgdb_buffer + 2, arg, 5);
			if (rval != 3) {
				kprintf("kgdb_trap: invalid param\n");
				break;
			}
			if (kgdb_buffer[1] == 'z') {
				remove_bp(arg[1]);
			} else {
				rval = setup_bp(arg[1]);
				if (rval) {
					kgdb_put_packet("E10");
				}
			}
			kgdb_put_packet("OK");
			break;
			/* read mem */
		case 'm':
			rval = kgdb_parse(kgdb_buffer + 2, arg, 5);
			if (rval != 2) {
				kprintf("kgdb_trap: invalid param\n");
				break;
			}
			if (kgdb_sendmem(kgdb_buffer, arg[0], arg[1])) {
				kgdb_put_packet("E13");
			}
			break;
			/* modify mem */
		case 'M':
			rval = kgdb_parse(kgdb_buffer + 2, arg, 2);
			if (rval != 2) {
				kprintf("kgdb_trap: invalid param\n");
				break;
			}
			if (kgdb_changemem(kgdb_buffer + 2, arg[0], arg[1])) {
				kgdb_put_packet("E11");
			}
			break;
			/* continue */
		case 'D':
		case 'k':
		case 'c':
		case 's':
			rval = kgdb_put_packet("+");
			goto cont_kernel;
		default:
			kprintf("kgdb_trap: invalid command '%c'\n",
				kgdb_buffer[1]);
		}
	}

cont_kernel:
	end_debug();
	/* soft bp */
	if (!compilation_bp) {
		/* one shot bp */
		remove_bp(pc);
		tf->tf_epc = pc;
	}
	activate_bps();
	trap_reentry = 0;
	return 0;
}
