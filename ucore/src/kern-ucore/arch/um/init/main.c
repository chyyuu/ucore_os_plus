#include <arch.h>
#include <console.h>
#include <intr.h>
#include <ide.h>
#include <umclock.h>
#include <host_signal.h>
#include <pmm.h>
#include <kio.h>
#include <vmm.h>
#include <proc.h>
#include <sched.h>
#include <swap.h>
#include <fs.h>
#include <mp.h>

void host_exit(int sig)
{
	cons_dtor();
	syscall1(__NR_exit, 0);
}

/**
 * The entry.
 */
int main(int argc, char *argv[], char *envp[])
{
	if (ginfo->status == STATUS_DEBUG)
		raise(SIGTRAP);

	cons_init();

	const char *message = "(THU.CST) os is loading ...";
	kprintf("%s\n\n", message);

	intr_init();
	ide_init();

	host_signal_init();

	/* Only to initialize lcpu_count. */
	mp_init();

	pmm_init();
	pmm_init_ap();
	vmm_init();
	sched_init();
	proc_init();

	swap_init();
	fs_init();
	sync_init();

	umclock_init();
	cpu_idle();

	host_exit(SIGINT);

	return 0;
}
