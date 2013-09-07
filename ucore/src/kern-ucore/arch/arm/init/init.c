#include <board.h>
#include <types.h>
#include <stdio.h>
#include <kio.h>
#include <kdebug.h>
#include <string.h>
#include <console.h>
#include <picirq.h>
#include <trap.h>
#include <clock.h>
#include <intr.h>
#include <pmm.h>
#include <vmm.h>
#include <ide.h>
#include <swap.h>
#include <proc.h>
#include <assert.h>
#include <atomic.h>
#include <mp.h>
#include <sched.h>
#include <fs.h>
#include <sync.h>
#include <ramdisk.h>
#include <kgdb-stub.h>
#include <module.h>
#include <dde_kit/dde_kit.h>

#ifdef UCONFIG_HAVE_YAFFS2
#include <yaffs2_direct/yaffsfs.h>
#endif

#ifdef printf
#undef printf
#endif

#ifdef DEBUG
#define _PROBE_() kprintf("PROBE %s: %d\n", __FILE__, __LINE__);
#else
#define _PROBE_()
#endif

// Very important:
// At the boot, exceptions handlers have not been defined
// Copy the address of exceptions handlers (defined in vectors.S) to 0x24 
// (or Ox20 if we decide to change reset handler)
// XXX it works only when 0x0 is a sram/sdram
extern char __vector_table, __vector_table_end;
static inline void exception_vector_init(void)
{
	memcpy((void *)SDRAM0_START, (void *)&__vector_table,
	       &__vector_table_end - &__vector_table);
}

int kern_init(void) __attribute__ ((noreturn));

#ifdef UCONFIG_HAVE_YAFFS2
static void test_yaffs()
{
//  mtd_erase_partition(get_nand_chip(),"data");
	yaffs_mount("/data");
	kprintf("DUMP\n");
	yaffs_dump_dir("/data");
	yaffs_mkdir("/data/td", S_IREAD | S_IWRITE);
	yaffs_mkdir("/data/td/d1", S_IWRITE | S_IREAD);
	//int fd = yaffs_unlink("/data/test.txt");
	//kprintf("@@@@@@@ fd %d %s\n", fd, yaffs_error_to_str(yaffs_get_error())); 
	int fd = yaffs_open("/data/test.txt", O_RDONLY, 0);
	//int fd = yaffs_open("/data/td/test.txt",O_RDWR|O_CREAT, S_IREAD|S_IWRITE);
	kprintf("@@@@ fd %d %s\n", fd, yaffs_error_to_str(yaffs_get_error()));
	char buf[32];
	yaffs_write(fd, "abc", 3);
	int ret = yaffs_read(fd, buf, 5);
	buf[ret] = 0;
	buf[16] = 0;
	kprintf("@@@@ read: %d  %s\n", ret, buf);
	yaffs_close(fd);
	yaffs_sync("/data");
	kprintf("DUMP\n");
	yaffs_dump_dir("/data");
	yaffs_unmount("/data");

}
#endif

static void __test_bp()
{
	kgdb_breakpoint();
}

static void __dummy_foo()
{
	static int i = 0;
	i++;
	kprintf("__dummy_foo: testbp %d\n", i);
}

static void check_bp()
{
	kprintf("check compilation bp...\n");
	__test_bp();
	kprintf("check setup bp...\n");
	setup_bp((uint32_t) __dummy_foo);
	kprintf("calling __dummy_foo\n");
	__dummy_foo();
	kprintf("check bp done...\n");
}

int kern_init(void)
{
	extern char edata[], end[];
	memset(edata, 0, end - edata);

	//char *p = 0xc81aa000;
//	*p = 0;

	exception_vector_init();
//	pmm_init();		// init physical memory management
//	pmm_init_ap();
	board_init_early();

#ifdef UCONFIG_HAVE_RAMDISK
	check_initrd();
#endif

	const char *message = "(THU.CST) os is loading ...";
	kprintf("%s\n\n", message);

	//kgdb_init();
	//check_bp();

	/* Only to initialize lcpu_count. */
	mp_init();

	print_kerninfo();

	pmm_init();		// init physical memory management
	pmm_init_ap();
#ifdef UCONFIG_HAVE_LINUX_DDE_BASE
	dde_call_mapio_early();
#endif

	kprintf("pmm_init() done.\n");

	vmm_init();		// init virtual memory management
	_PROBE_();

	cons_init();		// init the console

	sched_init();		// init scheduler
	_PROBE_();
	proc_init();		// init process table
	_PROBE_();
	sync_init();		// init sync struct

	ide_init();		// init ide devices
	_PROBE_();
	//swap_init();                // init swap
	_PROBE_();
	fs_init();		// init fs
	_PROBE_();

	intr_enable();		// enable irq interrupt

#ifdef UCONFIG_HAVE_LINUX_DDE_BASE
	calibrate_delay();
	dde_init();
#endif
#ifdef UCONFIG_HAVE_LINUX_DDE36_BASE
	dde_kit_init();
#endif

#ifdef UCONFIG_HAVE_YAFFS2
#ifdef HAS_NANDFLASH
	yaffs_start_up();
	//test_yaffs();
	yaffs_vfs_init();
#else
	kprintf("init: using yramsim\n");
	//emulated yaffs start up
#ifdef UCONFIG_HAVE_YAFFS2_RAMDISK
	yaffsfs_OSInitialisation();
	yramsim_CreateRamSim("data", 1, 20, 2, 16);
	//test_yaffs();

	yaffs_vfs_init();
#endif

#endif
#endif /* UCONFIG_HAVE_YAFFS2 */

#ifdef UCONFIG_HAVE_LINUX_DDE_BASE
	ucore_vfs_add_device("fb0", 29, 0);
	//ucore_vfs_add_device("input0", 13, 0);
	ucore_vfs_add_device("event0", 13, 64);
	ucore_vfs_add_device("hzfchar", 222, 0);
#endif

	enable_timer_list();
	print_pgdir(kprintf);
	cpu_idle();		// run idle process
}
