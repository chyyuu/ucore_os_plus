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

/* alloc percpu var in the corresponding    NUMA nodes */
void percpu_init(void)
{
	size_t percpu_size = ROUNDUP(__percpu_end - __percpu_start, CACHELINE);
	if(!percpu_size || sysconf.lcpu_count<=1)
		return;
	int i, j;
	for(i=0; i<sysconf.lnuma_count;i++){
		struct numa_node *node = &numa_nodes[i];
		int nr_cpus = node->nr_cpus;
		for(j=0;j<node->nr_cpus;j++){
			if(node->cpu_ids[j] == myid()){
				nr_cpus--;
				break;
			}
		}
		size_t totalsize = percpu_size * nr_cpus;
		unsigned int pages = ROUNDUP_DIV(totalsize, PGSIZE);
		if(!pages)
			continue;
		kprintf("percpu_init: node%d alloc %d pages for percpu variables\n", i, pages);
		struct Page *p = alloc_pages_numa(node, pages);
		assert(p != NULL);
		void *kva = page2kva(p);
		/* zero it out */
		memset(kva, 0, pages * PGSIZE);
		for(j=0;j<node->nr_cpus;j++, kva += percpu_size){
			if(node->cpu_ids[j] == myid())
				continue;
			percpu_offsets[node->cpu_ids[j]] = kva;
		}
	}
	for(i=0;i<sysconf.lcpu_count;i++){
		kprintf("percpu_init: cpu%d get 0x%016llx\n", i, percpu_offsets[i]);
	}
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
	gdt_init(per_cpu_ptr(cpus, bcpuid));
	tls_init(per_cpu_ptr(cpus, bcpuid));
	kprintf("CPU%d alive\n", myid());
	/* load new pagetable(shared with bsp) */
	pmm_init_ap();
	idt_init();		// init interrupt descriptor table
#ifdef UCONFIG_ENABLE_IPI
	ipi_init();
#endif

	/* test pmm */
	struct Page *p = alloc_pages(2);
	kprintf("I'm %d, get 0x%016llx(PA)\n", myid(), page2pa(p));
	free_pages(p, 2);

	lapic_init();
	proc_init_ap();

	atomic_inc(&bsync); /* let BSP know we are up */

	intr_enable();		// enable irq interrupt
	cpu_idle();
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

	stack = (char*)page2kva(alloc_pages_cpu(c, KSTACKPAGE));
	assert(stack != NULL);

	kprintf("LAPIC %d, CODE %p PA: %p, STACK: %p\n", c->hwid, code, PADDR_DIRECT(code), stack);
	warmreset(PADDR_DIRECT(code));

	*(uint32_t*)(code-4) = (uint32_t)PADDR_DIRECT(&apstart);
	*(uint64_t*)(code-12) = (uint64_t)stack + KSTACKSIZE;
	*(uint64_t*)(code-20) = (uint64_t)boot_cr3;
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
	struct cpu *cpu = mycpu();
	uintptr_t new_cr3;
	if (mm != NULL && mm->pgdir != NULL)
		new_cr3 = PADDR(mm->pgdir);
	else
		new_cr3 = boot_cr3;

	mp_lcr3(new_cr3);
}

pgd_t *mpti_pgdir;
uintptr_t mpti_la;
volatile int mpti_end;

static void dump_processors()
{
	int i;
	for(i=0; i < sysconf.lcpu_count; i++){
		struct cpu *cpu = per_cpu_ptr(cpus, i);
		kprintf("CPU%d: pid %d, name: %s\n", i, cpu->__current?cpu->__current->pid:-1,
				cpu->__current?cpu->__current->name:NULL);
	}
}

/* XXX naive */
static void shootdown_tlb_all(pgd_t *pgdir)
{
	int i;
	//dump_processors();
	for(i=0;i<sysconf.lcpu_count;i++){
		struct cpu *cpu = per_cpu_ptr(cpus, i);
		if(cpu->id == myid())
			continue;
		if(cpu->arch_data.tlb_cr3 != PADDR(pgdir))
			continue;
		//kprintf("XX_TLB_SHUTDOWN %d %d\n", myid(), i);
		lapic_send_ipi(cpu, T_TLBFLUSH);
	}
}

void mp_tlb_invalidate(pgd_t * pgdir, uintptr_t la)
{
	tlb_invalidate(pgdir, la);
	shootdown_tlb_all(pgdir);
}

void mp_tlb_update(pgd_t * pgdir, uintptr_t la)
{
	tlb_update(pgdir, la);
	shootdown_tlb_all(pgdir);
}

void fire_ipi_one(int cpuid)
{
	lapic_send_ipi(per_cpu_ptr(cpus, cpuid), T_IPICALL);
}

