#include <mp.h>
#include <pmm.h>
#include <trap.h>
#include <spinlock.h>
#include <proc.h>
#include <kio.h>
#include <assert.h>
#include <arch.h>
#include <vmm.h>
#include <percpu.h>
#include <lapic.h>
#include <sysconf.h>

void *percpu_offsets[NCPU];
DEFINE_PERCPU_NOINIT(struct cpu, cpus);

#if 0
#define mp_debug(a ...) kprintf(a)
#else
#define mp_debug(a ...)
#endif

void
tls_init(struct cpu *c)
{
	// Initialize cpu-local storage.
	writegs(KERNEL_DS);
	/* gs base shadow reg in msr */
	writemsr(MSR_GS_BASE, (uint64_t)&c->cpu);
	writemsr(MSR_GS_KERNBASE, (uint64_t)&c->cpu);
	c->cpu = c;
}

/* TODO alloc percpu var in the corresponding    NUMA nodes */
void percpu_init(void)
{
	size_t percpu_size = ROUNDUP(__percpu_end - __percpu_start, CACHELINE);
	if(!percpu_size || sysconf.lcpu_count<=1)
		return;
	int i;
	size_t totalsize = percpu_size * (sysconf.lcpu_count - 1);
	unsigned int pages = ROUNDUP_DIV(totalsize, PGSIZE);
	kprintf("percpu_init: alloc %d pages for percpu variables\n", pages);
	struct Page *p = alloc_pages(pages);
	assert(p != NULL);
	void *kva = page2kva(p);
	/* zero it out */
	memset(kva, 0, pages * PGSIZE);
	for(i=1;i<sysconf.lcpu_count;i++, kva += percpu_size)
		percpu_offsets[i] = kva;
}


#define IO_RTC  0x70
static void warmreset(uint32_t addr)
{
	volatile uint16_t *wrv;

	// "The BSP must initialize CMOS shutdown code to 0AH
	// and the warm reset vector (DWORD based at 40:67) to point at
	// the AP startup code prior to the [universal startup algorithm]."
	outb(IO_RTC, 0xF);  // offset 0xF is shutdown code
	outb(IO_RTC+1, 0x0A);
	wrv = (uint16_t*)VADDR_DIRECT(0x40<<4 | 0x67);  // Warm reset vector
	wrv[0] = 0;
	wrv[1] = addr >> 4;
}

static void rstrreset(void)
{
	volatile uint16_t *wrv;

	// Paranoid: set warm reset code and vector back to defaults
	outb(IO_RTC, 0xF);
	outb(IO_RTC+1, 0);
	wrv = (uint16_t*)VADDR_DIRECT(0x40<<4 | 0x67);
	wrv[0] = 0;
	wrv[1] = 0;
}

static int bcpuid;
static atomic_t bsync = ATOMIC_INIT(0);

void ap_init(void)
{
	tls_init(per_cpu_ptr(cpus, bcpuid));
	kprintf("CPU%d alive\n", myid());
	/* load new pagetable(shared with bsp) */
	pmm_init_ap();
	idt_init();		// init interrupt descriptor table

	atomic_inc(&bsync); /* let BSP know we are up */
	while(1);
}

void cpu_up(int id)
{
	extern char _bootother_start[];
	extern uint64_t _bootother_size;
	extern void (*apstart)(void);
	char *stack;

	unsigned char *code = (unsigned char*)VADDR_DIRECT(0x7000);
	struct cpu *c = per_cpu_ptr(cpus, id);
	assert(c->id != myid());
	assert(c->id == id);
	memcpy(code, _bootother_start, _bootother_size);

	stack = (char*)page2kva(alloc_pages(KSTACKPAGE));
	assert(stack != NULL);

	kprintf("LAPIC %d, CODE %p PA: %p, STACK: %p\n", c->hwid, code, PADDR_DIRECT(code), stack);
	warmreset(PADDR_DIRECT(code));

	*(uint32_t*)(code-4) = (uint32_t)PADDR_DIRECT(&apstart);
	*(uint64_t*)(code-12) = (uint64_t)stack + KSTACKSIZE;
	// bootother.S sets this to 0x0a55face early on
	*(uint32_t*)(code-64) = 0;
	bcpuid = c->id;
	atomic_set(&bsync, 0);

	lapic_start_ap(c, PADDR_DIRECT(code));

	while(atomic_read(&bsync) == 0)
		nop_pause();
	rstrreset();
}


void kern_enter(int source)
{
}

void kern_leave(void)
{
}

void mp_set_mm_pagetable(struct mm_struct *mm)
{
	if (mm != NULL && mm->pgdir != NULL)
		lcr3(PADDR(mm->pgdir));
	else
		lcr3(boot_cr3);
}

pgd_t *mpti_pgdir;
uintptr_t mpti_la;
volatile int mpti_end;

void mp_tlb_invalidate(pgd_t * pgdir, uintptr_t la)
{
	tlb_invalidate(pgdir, la);
}

void mp_tlb_update(pgd_t * pgdir, uintptr_t la)
{
	tlb_update(pgdir, la);
}
