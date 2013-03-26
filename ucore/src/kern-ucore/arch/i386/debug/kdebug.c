#include <types.h>
#include <arch.h>
#include <stab.h>
#include <stdio.h>
#include <string.h>
#include <memlayout.h>
#include <sync.h>
#include <vmm.h>
#include <proc.h>
#include <kdebug.h>
#include <monitor.h>
#include <assert.h>
#include <kio.h>
#include <mp.h>

#define STACKFRAME_DEPTH 20

extern const struct stab __STAB_BEGIN__[];	// beginning of stabs table
extern const struct stab __STAB_END__[];	// end of stabs table
extern const char __STABSTR_BEGIN__[];	// beginning of string table
extern const char __STABSTR_END__[];	// end of string table

/* debug information about a particular instruction pointer */
struct eipdebuginfo {
	const char *eip_file;	// source code filename for eip
	int eip_line;		// source code line number for eip
	const char *eip_fn_name;	// name of function containing eip
	int eip_fn_namelen;	// length of function's name
	uintptr_t eip_fn_addr;	// start address of function
	int eip_fn_narg;	// number of function arguments
};

/* user STABS data structure  */
struct userstabdata {
	const struct stab *stabs;
	const struct stab *stab_end;
	const char *stabstr;
	const char *stabstr_end;
};

/* *
 * stab_binsearch - according to the input, the initial value of
 * range [*@region_left, *@region_right], find a single stab entry
 * that includes the address @addr and matches the type @type,
 * and then save its boundary to the locations that pointed
 * by @region_left and @region_right.
 *
 * Some stab types are arranged in increasing order by instruction address.
 * For example, N_FUN stabs (stab entries with n_type == N_FUN), which
 * mark functions, and N_SO stabs, which mark source files.
 *
 * Given an instruction address, this function finds the single stab entry
 * of type @type that contains that address.
 *
 * The search takes place within the range [*@region_left, *@region_right].
 * Thus, to search an entire set of N stabs, you might do:
 *
 *      left = 0;
 *      right = N - 1;    (rightmost stab)
 *      stab_binsearch(stabs, &left, &right, type, addr);
 *
 * The search modifies *region_left and *region_right to bracket the @addr.
 * *@region_left points to the matching stab that contains @addr,
 * and *@region_right points just before the next stab.
 * If *@region_left > *region_right, then @addr is not contained in any
 * matching stab.
 *
 * For example, given these N_SO stabs:
 *      Index  Type   Address
 *      0      SO     f0100000
 *      13     SO     f0100040
 *      117    SO     f0100176
 *      118    SO     f0100178
 *      555    SO     f0100652
 *      556    SO     f0100654
 *      657    SO     f0100849
 * this code:
 *      left = 0, right = 657;
 *      stab_binsearch(stabs, &left, &right, N_SO, 0xf0100184);
 * will exit setting left = 118, right = 554.
 * */
static void
stab_binsearch(const struct stab *stabs, int *region_left, int *region_right,
	       int type, uintptr_t addr)
{
	int l = *region_left, r = *region_right, any_matches = 0;

	while (l <= r) {
		int true_m = (l + r) / 2, m = true_m;

		// search for earliest stab with right type
		while (m >= l && stabs[m].n_type != type) {
			m--;
		}
		if (m < l) {	// no match in [l, m]
			l = true_m + 1;
			continue;
		}
		// actual binary search
		any_matches = 1;
		if (stabs[m].n_value < addr) {
			*region_left = m;
			l = true_m + 1;
		} else if (stabs[m].n_value > addr) {
			*region_right = m - 1;
			r = m - 1;
		} else {
			// exact match for 'addr', but continue loop to find
			// *region_right
			*region_left = m;
			l = m;
			addr++;
		}
	}

	if (!any_matches) {
		*region_right = *region_left - 1;
	} else {
		// find rightmost region containing 'addr'
		l = *region_right;
		for (; l > *region_left && stabs[l].n_type != type; l--)
			/* do nothing */ ;
		*region_left = l;
	}
}

/* *
 * debuginfo_eip - Fill in the @info structure with information about
 * the specified instruction address, @addr.  Returns 0 if information
 * was found, and negative if not.  But even if it returns negative it
 * has stored some information into '*info'.
 * */
int debuginfo_eip(uintptr_t addr, struct eipdebuginfo *info)
{
	const struct stab *stabs, *stab_end;
	const char *stabstr, *stabstr_end;

	info->eip_file = "<unknown>";
	info->eip_line = 0;
	info->eip_fn_name = "<unknown>";
	info->eip_fn_namelen = 9;
	info->eip_fn_addr = addr;
	info->eip_fn_narg = 0;

	// find the relevant set of stabs
	if (addr >= KERNBASE) {
		stabs = __STAB_BEGIN__;
		stab_end = __STAB_END__;
		stabstr = __STABSTR_BEGIN__;
		stabstr_end = __STABSTR_END__;
	} else {
		// user-program linker script, tools/user.ld puts the information about the
		// program's stabs (included __STAB_BEGIN__, __STAB_END__, __STABSTR_BEGIN__,
		// and __STABSTR_END__) in a structure located at virtual address USTAB.
		const struct userstabdata *usd = (struct userstabdata *)USTAB;

		// make sure that debugger (current process) can access this memory
		struct mm_struct *mm;
		if (current == NULL || (mm = current->mm) == NULL) {
			return -1;
		}
		if (!user_mem_check
		    (mm, (uintptr_t) usd, sizeof(struct userstabdata), 0)) {
			return -1;
		}

		stabs = usd->stabs;
		stab_end = usd->stab_end;
		stabstr = usd->stabstr;
		stabstr_end = usd->stabstr_end;

		// make sure the STABS and string table memory is valid
		if (!user_mem_check
		    (mm, (uintptr_t) stabs,
		     (uintptr_t) stab_end - (uintptr_t) stabs, 0)) {
			return -1;
		}
		if (!user_mem_check
		    (mm, (uintptr_t) stabstr, stabstr_end - stabstr, 0)) {
			return -1;
		}
	}

	// String table validity checks
	if (stabstr_end <= stabstr || stabstr_end[-1] != 0) {
		return -1;
	}
	// Now we find the right stabs that define the function containing
	// 'eip'.  First, we find the basic source file containing 'eip'.
	// Then, we look in that source file for the function.  Then we look
	// for the line number.

	// Search the entire set of stabs for the source file (type N_SO).
	int lfile = 0, rfile = (stab_end - stabs) - 1;
	stab_binsearch(stabs, &lfile, &rfile, N_SO, addr);
	if (lfile == 0)
		return -1;

	// Search within that file's stabs for the function definition
	// (N_FUN).
	int lfun = lfile, rfun = rfile;
	int lline, rline;
	stab_binsearch(stabs, &lfun, &rfun, N_FUN, addr);

	if (lfun <= rfun) {
		// stabs[lfun] points to the function name
		// in the string table, but check bounds just in case.
		if (stabs[lfun].n_strx < stabstr_end - stabstr) {
			info->eip_fn_name = stabstr + stabs[lfun].n_strx;
		}
		info->eip_fn_addr = stabs[lfun].n_value;
		addr -= info->eip_fn_addr;
		// Search within the function definition for the line number.
		lline = lfun;
		rline = rfun;
	} else {
		// Couldn't find function stab!  Maybe we're in an assembly
		// file.  Search the whole file for the line number.
		info->eip_fn_addr = addr;
		lline = lfile;
		rline = rfile;
	}
	info->eip_fn_namelen =
	    strfind(info->eip_fn_name, ':') - info->eip_fn_name;

	// Search within [lline, rline] for the line number stab.
	// If found, set info->eip_line to the right line number.
	// If not found, return -1.
	stab_binsearch(stabs, &lline, &rline, N_SLINE, addr);
	if (lline <= rline) {
		info->eip_line = stabs[rline].n_desc;
	} else {
		return -1;
	}

	// Search backwards from the line number for the relevant filename stab.
	// We can't just use the "lfile" stab because inlined functions
	// can interpolate code from a different file!
	// Such included source files use the N_SOL stab type.
	while (lline >= lfile
	       && stabs[lline].n_type != N_SOL
	       && (stabs[lline].n_type != N_SO || !stabs[lline].n_value)) {
		lline--;
	}
	if (lline >= lfile && stabs[lline].n_strx < stabstr_end - stabstr) {
		info->eip_file = stabstr + stabs[lline].n_strx;
	}
	// Set eip_fn_narg to the number of arguments taken by the function,
	// or 0 if there was no containing function.
	if (lfun < rfun) {
		for (lline = lfun + 1;
		     lline < rfun && stabs[lline].n_type == N_PSYM; lline++) {
			info->eip_fn_narg++;
		}
	}
	return 0;
}

/* *
 * print_kerninfo - print the information about kernel, including the location
 * of kernel entry, the start addresses of data and text segements, the start
 * address of free memory and how many memory that kernel has used.
 * */
void print_kerninfo(void)
{
	extern char etext[], edata[], end[], kern_init[];
	kprintf("Special kernel symbols:\n");
	kprintf("  entry  0x%08x (phys)\n", kern_init);
	kprintf("  etext  0x%08x (phys)\n", etext);
	kprintf("  edata  0x%08x (phys)\n", edata);
	kprintf("  end    0x%08x (phys)\n", end);
	kprintf("Kernel executable memory footprint: %dKB\n",
		(end - kern_init + 1023) / 1024);
}

/* *
 * print_debuginfo - read and print the stat information for the address @eip,
 * and info.eip_fn_addr should be the first address of the related function.
 * */
void print_debuginfo(uintptr_t eip)
{
	struct eipdebuginfo info;
	if (debuginfo_eip(eip, &info) != 0) {
		kprintf("    <unknow>: -- 0x%08x --\n", eip);
	} else {
		char fnname[256];
		int j;
		for (j = 0; j < info.eip_fn_namelen; j++) {
			fnname[j] = info.eip_fn_name[j];
		}
		fnname[j] = '\0';
		kprintf("    %s:%d: %s+%d\n", info.eip_file, info.eip_line,
			fnname, eip - info.eip_fn_addr);
	}
}

static uint32_t read_eip(void) __attribute__ ((noinline));

static uint32_t read_eip(void)
{
	uint32_t eip;
	asm volatile ("movl 4(%%ebp), %0":"=r" (eip));
	return eip;
}

/* *
 * print_stackframe - print a list of the saved eip values from the nested 'call'
 * instructions that led to the current point of execution
 *
 * The x86 stack pointer, namely esp, points to the lowest location on the stack
 * that is currently in use. Everything below that location in stack is free. Pushing
 * a value onto the stack will invole decreasing the stack pointer and then writing
 * the value to the place that stack pointer pointes to. And popping a value do the
 * opposite.
 *
 * The ebp (base pointer) register, in contrast, is associated with the stack
 * primarily by software convention. On entry to a C function, the function's
 * prologue code normally saves the previous function's base pointer by pushing
 * it onto the stack, and then copies the current esp value into ebp for the duration
 * of the function. If all the functions in a program obey this convention,
 * then at any given point during the program's execution, it is possible to trace
 * back through the stack by following the chain of saved ebp pointers and determining
 * exactly what nested sequence of function calls caused this particular point in the
 * program to be reached. This capability can be particularly useful, for example,
 * when a particular function causes an assert failure or panic because bad arguments
 * were passed to it, but you aren't sure who passed the bad arguments. A stack
 * backtrace lets you find the offending function.
 *
 * The inline function read_ebp() can tell us the value of current ebp. And the
 * non-inline function read_eip() is useful, it can read the value of current eip,
 * since while calling this function, read_eip() can read the caller's eip from
 * stack easily.
 *
 * In print_debuginfo(), the function debuginfo_eip() can get enough information about
 * calling-chain. Finally print_stackframe() will trace and print them for debugging.
 *
 * Note that, the length of ebp-chain is limited. In boot/bootasm.S, before jumping
 * to the kernel entry, the value of ebp has been set to zero, that's the boundary.
 * */
void print_stackframe(void)
{
	uint32_t ebp = read_ebp(), eip = read_eip();

	int i, j;
	for (i = 0; ebp != 0 && i < 10; i++) {
		kprintf("ebp:0x%08x eip:0x%08x args:", ebp, eip);
		uint32_t *args = (uint32_t *) ebp + 2;
		for (j = 0; j < 4; j++) {
			kprintf("0x%08x ", args[j]);
		}
		kprintf("\n");
		print_debuginfo(eip - 1);
		eip = ((uint32_t *) ebp)[1];
		ebp = ((uint32_t *) ebp)[0];
	}
}

// below codes are added for proj4.3+, 
// and are used for ucore internal debugger support with hardware breakpoint&watchpoint functions

// used to save debug registers
static uint32_t local_dr[MAX_DR_NUM], status_dr, control_dr;

static unsigned int local_dr_counter[MAX_DR_NUM];

/* save_all_dr - save all debug registers to memory and then disable them */
static void save_all_dr(void)
{
	int i;
	for (i = 0; i < MAX_DR_NUM; i++) {
		local_dr[i] = read_dr(i);
	}
	status_dr = read_dr(DR_STATUS);
	control_dr = read_dr(DR_CONTROL);

	// disable breakpoints while debugger is running
	write_dr(DR_CONTROL, 0);

	// increase Debug Register Counter
	unsigned regnum;
	for (regnum = 0; regnum < MAX_DR_NUM; regnum++) {
		if (status_dr & (1 << regnum)) {
			local_dr_counter[regnum]++;
		}
	}
}

/* restore_all_dr - reset all debug registers and clear the status register DR6 */
static void restore_all_dr(void)
{
	int i;
	for (i = 0; i < MAX_DR_NUM; i++) {
		write_dr(i, local_dr[i]);
	}
	write_dr(DR_STATUS, 0);
	write_dr(DR_CONTROL, control_dr);
}

/* debug_enable_dr - set and enable debug register @regnum locally */
int
debug_enable_dr(unsigned regnum, uintptr_t addr, unsigned type, unsigned len)
{
	if (regnum < MAX_DR_NUM) {
		local_dr[regnum] = addr;
		local_dr_counter[regnum] = 0;
		unsigned shift = (regnum * 4) + 16;
		uint32_t mask = (0xF << shift);
		control_dr &= ~mask;
		control_dr |= ((type & 3) << shift);
		control_dr |= ((len & 3) << (shift + 2));
		control_dr |= (1 << (regnum * 2));
		return 0;
	}
	return -1;
}

/* debug_disable_dr - disable debug register @regnum locally */
int debug_disable_dr(unsigned regnum)
{
	if (regnum < MAX_DR_NUM) {
		unsigned shift = (regnum * 4) + 16;
		uint32_t mask = (0xF << shift);
		control_dr &= ~mask;
		control_dr &= ~(1 << (regnum * 2));
		return 0;
	}
	return -1;
}

static const char *BreakDescription[] = {
	"EXECUTE", "WRITE", "IOPORT", "READ/WRITE",
};

static const char *BreakLengthDescription[] = {
	"1-BYTE", "2-BYTE", "??????", "4-BYTE",
};

// mark if local_dr, status_dr and contorl_dr are valid
static bool is_dr_saved = 0;

/* debug_init - init all debug registers by using restore_dr */
void debug_init(void)
{
	memset(local_dr, 0, sizeof(local_dr));
	memset(local_dr_counter, 0, sizeof(local_dr_counter));
	control_dr = DR7_GEXACT | DR7_LEXACT;
	restore_all_dr();
}

/* debug_list_dr - list and print all debug registrs' value and type */
void debug_list_dr(void)
{
	bool has = 0;
	int regnum;
	for (regnum = 0; regnum < MAX_DR_NUM; regnum++) {
		if (control_dr & (1 << (regnum * 2))) {
			if (!has) {
				has = 1;
				kprintf
				    ("    Num Address    Type       Len    Count\n");
			}
			unsigned shift = (regnum * 4) + 16;
			unsigned type = ((control_dr >> shift) & 3);
			unsigned len = ((control_dr >> (shift + 2)) & 3);
			kprintf("    %1d   0x%08x %-10s %6s %d\n", regnum,
				local_dr[regnum], BreakDescription[type],
				BreakLengthDescription[len],
				local_dr_counter[regnum]);
		}
	}
	if (!has) {
		kprintf("no breakpoints or watchpoints.\n");
	}
}

/* *
 * debug_start - check if all debug registers have already been saved. When you
 * type 'step' to decide to run a single step, debug_end won't restore all these
 * debug registers, and keep the value of 'is_dr_saved'. When another debug interrupt
 * occurs, it may go into this function again.
 * */
static void debug_start(struct trapframe *tf)
{
	if (!is_dr_saved) {
		is_dr_saved = 1;
		save_all_dr();
	}
}

/* *
 * debug_end - restore all debug registers if necessory. Note that, if kernel
 * needs to run a single step, it should not restore them.
 * */
static void debug_end(struct trapframe *tf)
{
	if (!(tf->tf_eflags & FL_TF) && is_dr_saved) {
		is_dr_saved = 0;
		restore_all_dr();
	}
}

/* debug_monitor - goes into the debugger monitor, and type 'continue' to return */
void debug_monitor(struct trapframe *tf)
{
	assert(tf != NULL);
	bool intr_flag;
	local_intr_save(intr_flag);
	{
		debug_start(tf);
		kprintf("debug_monitor at:\n");
		print_debuginfo(tf->tf_eip);
		monitor(tf);
		debug_end(tf);
	}
	local_intr_restore(intr_flag);
}
