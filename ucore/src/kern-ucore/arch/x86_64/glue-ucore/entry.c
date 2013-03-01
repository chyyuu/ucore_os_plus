#include <slab.h>
#include <vmm.h>
#include <pmm.h>
#include <slab.h>
#include <trap.h>
#include <sched.h>
#include <proc.h>
#include <swap.h>
#include <fs.h>
#include <clock.h>
#include <intr.h>
#include <spinlock.h>
#include <mp.h>
#include <ide.h>
#include <trap.h>
#include <entry.h>

static spinlock_s init_lock;
volatile int init_finished = 0;

void __kern_entry(void)
{
	mp_init();

	if (pls_read(lcpu_idx) == 0) {
		spinlock_acquire(&init_lock);

		pmm_init();

		pmm_init_ap();
		slab_init();

		trap_init();
		vmm_init();	// init virtual memory management

		sched_init();	// init scheduler
		proc_init();	// init process table

		sync_init();	// init sync struct

		ide_init();
		swap_init();	// init swap
		fs_init();	// init fs

		clock_init();

		init_finished = 1;
		spinlock_release(&init_lock);
	} else {
		while (init_finished == 0) ;
		spinlock_acquire(&init_lock);

		pmm_init_ap();
		trap_init_ap();
		proc_init_ap();
		clock_init_ap();

		++init_finished;

		spinlock_release(&init_lock);
	}

	int lcpu_count = pls_read(lcpu_count);
	while (init_finished != lcpu_count) ;

	intr_enable();
	cpu_idle();		// run idle process

	while (1) ;
}
