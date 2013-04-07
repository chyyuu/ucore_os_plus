#
# Chen Yuheng 2012/3
#

#include <mmu.h>
#include <memlayout.h>
#include <board.h>
#include <pgtable-hwdef.h>
#include <mmu.h>


#define PAGE_OFFSET		0xc0000000
#define TEXT_OFFSET		0x8000
#define PHYS_OFFSET		UCONFIG_DRAM_START


#define __virt_to_phys(x)	((x) - PAGE_OFFSET + PHYS_OFFSET)
#define __phys_to_virt(x)	((x) - PHYS_OFFSET + PAGE_PFFSET)

#if (SDRAM0_START & 0x001fffff)
#error "SDRAM0_START must be at an even 2MiB boundary!"
#endif


#ifdef CONFIG_XIP_KERNEL
#define KERNEL_START	XIP_VIRT_ADDR(CONFIG_XIP_PHYS_ADDR)
#define KERNEL_END	_edata_loc
#else
#define KERNEL_START	KERNEL_RAM_VADDR
#define KERNEL_END	_end
#endif


#define KERNEL_RAM_VADDR	(PAGE_OFFSET + TEXT_OFFSET)
##if (KERNEL_RAM_VADDR & 0xffff) != 0x8000
##error KERNEL_RAM_VADDR must start at 0xXXXX8000
##endif






.text
.equ DISABLE_IRQ, 0x80
.equ DISABLE_FIQ, 0x40
.equ SYS_MOD, 0x1f
.equ IRQ_MOD, 0x12
.equ FIQ_MOD, 0x11
.equ SVC_MOD, 0x13
.equ ABT_MOD, 0x17
.equ UND_MOD, 0x1b

.equ MEM_SIZE, SDRAM0_SIZE 
.equ TEXT_BASE, SDRAM0_START


	.macro	pgtbl, rd
	ldr	\rd, =(__virt_to_phys(KERNEL_RAM_VADDR - 0x4000))
	.endm


# assuming:
# MMU off, D-cache off
.section .entrytext, "a"
.globl kern_entry
kern_entry:
  mrs r0, cpsr
	bic r0, r0, #(DISABLE_FIQ|DISABLE_IRQ)
	msr cpsr, r0

# Stack inititialization - starts in SVC mode
# only 12bytes are used in these stacks, see trapentry.S
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|IRQ_MOD)
	ldr sp,=irq_stack
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|FIQ_MOD)
	ldr sp,=fiq_stack
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|ABT_MOD)
	ldr sp,=abt_stack
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|UND_MOD)
	ldr sp,=und_stack
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|SYS_MOD)
	ldr sp,=sys_stacktop
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|SVC_MOD)

#in SVC mode
relocated:
    ldr sp,=bootstacktop


#ifdef UCONFIG_FPU_ENABLE

    # enable the FPU
    mrc p15, 0, r0, c1, c0, 2
    orr r0, r0, #0x300000            /* single precision */
    orr r0, r0, #0xC00000            /* double precision */
    mcr p15, 0, r0, c1, c0, 2
    mov r0, #0x40000000
    fmxr fpexc,r0

#endif //UCONFIG_FPU_ENABLE



/* enable swp,flow prediction */
#ifdef __MACH_ARM_CORTEX_A9
  ldr r1, =(1<<11)|(1<<10)
  MCR p15, 0, r1, c1, c0, 0
#endif

#  b kern_init
.globl ttest
ttest:
	bl __create_pgtbl

#	ldr r13, kern_init
#	ldr r13, =kern_init

	b __enable_mmu

# now kernel stack is ready , call the first C function
    b kern_init

# should never get here
    spin:
    b spin

###################################################################

mm_mmuflag:
.long	PMD_TYPE_SECT | PMD_SECT_AP_WRITE | PMD_SECT_AP_READ
# | PMD_FLAGS_SMP
#.long   PMD_TYPE_SECT | PMD_SECT_BUFFERABLE | PMD_SECT_CACHEABLE | PMD_BIT4 | PMD_SECT_AP_WRITE | PMD_SECT_AP_READ
#.long   PMD_TYPE_SECT | PMD_BIT4 | PMD_SECT_AP_WRITE | PMD_SECT_AP_READ

control_access_value:
.long	DOM3CLT

control_access_mask:
.long	CHANGEALLDOM


#ifdef __MACH_ARM_ARMV6
mmu_value:
.long  ENABLEMMU | ENABLEICACHE | ENABLEDCACHE | ENABLEHIGHEVT | ENABLENEWPT
mmu_mask:
.long  CHANGEMMU | CHANGEICACHE | CHANGEDCACHE | CHANGEHIGHEVT | CHANGENEWPT
#elif defined __MACH_ARM_ARMV7
mmu_value:
.long	ENABLEMMU | ENABLEICACHE | ENABLEDCACHE | ENABLEHIGHEVT
mmu_mask:
.long	CHANGEMMU | CHANGEICACHE | CHANGEDCACHE | CHANGEHIGHEVT
#else
#error Unsupported arch
#endif

__create_pgtbl:
	pgtbl	r4

	mov r0, r4
	mov r3, #0
	add r6, r0, #0x4000
1:	str	r3, [r0], #4
	str	r3, [r0], #4
	str	r3, [r0], #4
	str	r3, [r0], #4
	teq	r0, r6
	bne	1b

#	mov r7, #mm_mmuflag
#	ldr r7, [r7, #0]
	ldr r7, =mm_mmuflag
	eor r7, r7, #0xc << 28
	ldr r7, [r7]

	# map 0x0 to 0x0
	ldr r6, =0
	orr r6, r6, r7
	str r6, [r4]

	# map first section
	mov r6, pc, lsr #20
	orr r3, r7, r6, lsl #20
	str r3, [r4, r6, lsl #2]

	# map first kernel section
	add r0, r4, #(KERNEL_RAM_VADDR & 0xff000000) >> 18
	str r3, [r0, #(KERNEL_RAM_VADDR & 0x00f00000) >> 18]!

#	ldr r6, =(end - PAGE_OFFSET - 1)
#	ldr r6, =(end - PAGE_OFFSET - 1)
	ldr r6, =(KMEMSIZE - 1)
	mov r6, r6, lsr #20

	#add some more memory for dynamically allocation
#	add r6, r6, #23

1:	add r3, r3, #1 << 20
	str r3, [r0, #4]!
	subs r6, r6, #1
	bgt 1b

	add r0, r4, #PAGE_OFFSET >> 18
	orr r6, r7, #PHYS_OFFSET
	str r6, [r0]

	# map io space to new page table
	ldr r6, =IO_SPACE_START
	mov r6, r6, lsr #20
	mov r0, r6
	orr r3, r7, r6, lsl #20
#	str r3, [r4, r6, lsl #2]

	add r0, r4, r0, lsl #2

	ldr r6, =IO_SPACE_SIZE
	mov r6, r6, lsr #20

1:	str r3, [r0]
	add r0, r0, #4
	add r3, r3, #1 << 20
	subs r6, r6, #1
	bgt 1b


	mov pc, lr


	.ltorg
	.align
__enable_mmu_loc:
	.long	.
	.long	__enable_mmu
	.long	__enable_mmu_end

/*
   r0, r1, r2
*/
__enable_mmu:

	mcr	p15, 0, r4, c2, c0, 0		@ load page table pointer

	#set domain access
	mrc p15, 0, r0, c3, c0, 0
	ldr r1, control_access_value
	ldr r2, control_access_mask
	mvn r2, r2
	and r0, r0, r2
	orr	r0, r0, r1
	mcr p15, 0, r0, c3, c0, 0

	#enable cache
	mrc p15, 0, r0, c1, c0, 0
	ldr r1, mmu_value
	ldr r2, mmu_mask
	mvn r2, r2
	and r0, r0, r2
	orr r0, r0, r1
	mcr p15, 0, r0, c1, c0, 0

	ldr r0, =kern_init

	# initialize the stack top
	ldr sp,=bootstacktop

	mov r0, r0
	mov r0, r0
	mov r0, r0

	# goto kern_init
	mov pc, r0

__enable_mmu_end:








    .data
    .align 6
# There might be a alignment problem, as should be aligned to a page size
irq_stack:
.space 64
irq_stacktop:
fiq_stack:
.space 64
fiq_stacktop:
abt_stack:
.space 64
abt_stacktop:
und_stack:
.space 64
und_stacktop:
sys_stack:
.space 64
sys_stacktop:
    .globl bootstack
bootstack:
    .space KSTACKSIZE
    .globl bootstacktop
bootstacktop:

/*
.section data_initpg, "a"
initpg:
	.space 4096*4
initpgend:
*/


/*
 * User helpers.
 *
 * These are segment of kernel provided user code reachable from user space
 * at a fixed address in kernel memory.  This is used to provide user space
 * with some operations which require kernel help because of unimplemented
 * native feature and/or instructions in many ARM CPUs. The idea is for
 * this code to be executed directly in user mode for best efficiency but
 * which is too intimate with the kernel counter part to be left to user
 * libraries.  In fact this code might even differ from one CPU to another
 * depending on the available  instruction set and restrictions like on
 * SMP systems.  In other words, the kernel reserves the right to change
 * this code as needed without warning. Only the entry points and their
 * results are guaranteed to be stable.
 *
 * Each segment is 32-byte aligned and will be moved to the top of the high
 * vector page.  New segments (if ever needed) must be added in front of
 * existing ones.  This mechanism should be used only for things that are
 * really small and justified, and not be abused freely.
 *
 * User space is expected to implement those things inline when optimizing
 * for a processor that has the necessary native support, but only if such
 * resulting binaries are already to be incompatible with earlier ARM
 * processors due to the use of unsupported instructions other than what
 * is provided here.  In other words don't make binaries unable to run on
 * earlier processors just for the sake of not using these kernel helpers
 * if your compiled code is not going to use the new instructions for other
 * purpose.
 */
/* THUMB(	.arm	)*/

	.macro	usr_ret, reg
#ifdef CONFIG_ARM_THUMB
	bx	\reg
#else
	mov	pc, \reg
#endif
	.endm

	.align	5
	.globl	__kuser_helper_start
__kuser_helper_start:

/*
 * Reference prototype:
 *
 *	void __kernel_memory_barrier(void)
 *
 * Input:
 *
 *	lr = return address
 *
 * Output:
 *
 *	none
 *
 * Clobbered:
 *
 *	none
 *
 * Definition and user space usage example:
 *
 *	typedef void (__kernel_dmb_t)(void);
 *	#define __kernel_dmb (*(__kernel_dmb_t *)0xffff0fa0)
 *
 * Apply any needed memory barrier to preserve consistency with data modified
 * manually and __kuser_cmpxchg usage.
 *
 * This could be used as follows:
 *
 * #define __kernel_dmb() \
 *         asm volatile ( "mov r0, #0xffff0fff; mov lr, pc; sub pc, r0, #95" \
 *	        : : : "r0", "lr","cc" )
 */

__kuser_memory_barrier:				@ 0xffff0fa0
	/*smp_dmb	arm*/
	usr_ret	lr

	.align	5

/*
 * Reference prototype:
 *
 *	int __kernel_cmpxchg(int oldval, int newval, int *ptr)
 *
 * Input:
 *
 *	r0 = oldval
 *	r1 = newval
 *	r2 = ptr
 *	lr = return address
 *
 * Output:
 *
 *	r0 = returned value (zero or non-zero)
 *	C flag = set if r0 == 0, clear if r0 != 0
 *
 * Clobbered:
 *
 *	r3, ip, flags
 *
 * Definition and user space usage example:
 *
 *	typedef int (__kernel_cmpxchg_t)(int oldval, int newval, int *ptr);
 *	#define __kernel_cmpxchg (*(__kernel_cmpxchg_t *)0xffff0fc0)
 *
 * Atomically store newval in *ptr if *ptr is equal to oldval for user space.
 * Return zero if *ptr was changed or non-zero if no exchange happened.
 * The C flag is also set if *ptr was changed to allow for assembly
 * optimization in the calling code.
 *
 * Notes:
 *
 *    - This routine already includes memory barriers as needed.
 *
 * For example, a user space atomic_add implementation could look like this:
 *
 * #define atomic_add(ptr, val) \
 *	({ register unsigned int *__ptr asm("r2") = (ptr); \
 *	   register unsigned int __result asm("r1"); \
 *	   asm volatile ( \
 *	       "1: @ atomic_add\n\t" \
 *	       "ldr	r0, [r2]\n\t" \
 *	       "mov	r3, #0xffff0fff\n\t" \
 *	       "add	lr, pc, #4\n\t" \
 *	       "add	r1, r0, %2\n\t" \
 *	       "add	pc, r3, #(0xffff0fc0 - 0xffff0fff)\n\t" \
 *	       "bcc	1b" \
 *	       : "=&r" (__result) \
 *	       : "r" (__ptr), "rIL" (val) \
 *	       : "r0","r3","ip","lr","cc","memory" ); \
 *	   __result; })
 */

__kuser_cmpxchg:				@ 0xffff0fc0

#if defined(CONFIG_NEEDS_SYSCALL_FOR_CMPXCHG)

	/*
	 * Poor you.  No fast solution possible...
	 * The kernel itself must perform the operation.
	 * A special ghost syscall is used for that (see traps.c).
	 */
	stmfd	sp!, {r7, lr}
	ldr	r7, 1f			@ it's 20 bits
	swi	__ARM_NR_cmpxchg
	ldmfd	sp!, {r7, pc}
1:	.word	__ARM_NR_cmpxchg

#elif __LINUX_ARM_ARCH__ < 6

#ifdef CONFIG_MMU

	/*
	 * The only thing that can break atomicity in this cmpxchg
	 * implementation is either an IRQ or a data abort exception
	 * causing another process/thread to be scheduled in the middle
	 * of the critical sequence.  To prevent this, code is added to
	 * the IRQ and data abort exception handlers to set the pc back
	 * to the beginning of the critical section if it is found to be
	 * within that critical section (see kuser_cmpxchg_fixup).
	 */
1:	ldr	r3, [r2]			@ load current val
	subs	r3, r3, r0			@ compare with oldval
2:	streq	r1, [r2]			@ store newval if eq
	rsbs	r0, r3, #0			@ set return val and C flag
	usr_ret	lr

	.text
kuser_cmpxchg_fixup:
	@ Called from kuser_cmpxchg_check macro.
	@ r2 = address of interrupted insn (must be preserved).
	@ sp = saved regs. r7 and r8 are clobbered.
	@ 1b = first critical insn, 2b = last critical insn.
	@ If r2 >= 1b and r2 <= 2b then saved pc_usr is set to 1b.
	mov	r7, #0xffff0fff
	sub	r7, r7, #(0xffff0fff - (0xffff0fc0 + (1b - __kuser_cmpxchg)))
	subs	r8, r2, r7
	rsbcss	r8, r8, #(2b - 1b)
	strcs	r7, [sp, #S_PC]
	mov	pc, lr
	.previous

#else
#warning "NPTL on non MMU needs fixing"
	mov	r0, #-1
	adds	r0, r0, #0
	usr_ret	lr
#endif

#else

	/*smp_dmb	arm*/
1:	ldrex	r3, [r2]
	subs	r3, r3, r0
	strexeq	r3, r1, [r2]
	teqeq	r3, #1
	beq	1b
	rsbs	r0, r3, #0
	/* beware -- each __kuser slot must be 8 instructions max */
	/*ALT_SMP(b	__kuser_memory_barrier)*/
	/*ALT_UP(usr_ret	lr)*/

#endif

	.align	5

/*
 * Reference prototype:
 *
 *	int __kernel_get_tls(void)
 *
 * Input:
 *
 *	lr = return address
 *
 * Output:
 *
 *	r0 = TLS value
 *
 * Clobbered:
 *
 *	none
 *
 * Definition and user space usage example:
 *
 *	typedef int (__kernel_get_tls_t)(void);
 *	#define __kernel_get_tls (*(__kernel_get_tls_t *)0xffff0fe0)
 *
 * Get the TLS value as previously set via the __ARM_NR_set_tls syscall.
 *
 * This could be used as follows:
 *
 * #define __kernel_get_tls() \
 *	({ register unsigned int __val asm("r0"); \
 *         asm( "mov r0, #0xffff0fff; mov lr, pc; sub pc, r0, #31" \
 *	        : "=r" (__val) : : "lr","cc" ); \
 *	   __val; })
 */

__kuser_get_tls:				@ 0xffff0fe0
	ldr	r0, [pc, #(16 - 8)]	@ read TLS, set in kuser_get_tls_init
	usr_ret	lr
	mrc	p15, 0, r0, c13, c0, 3	@ 0xffff0fe8 hardware TLS code
	.rep	4
	.word	0			@ 0xffff0ff0 software TLS value, then
	.endr				@ pad up to __kuser_helper_version

/*
 * Reference declaration:
 *
 *	extern unsigned int __kernel_helper_version;
 *
 * Definition and user space usage example:
 *
 *	#define __kernel_helper_version (*(unsigned int *)0xffff0ffc)
 *
 * User space may read this to determine the curent number of helpers
 * available.
 */

__kuser_helper_version:				@ 0xffff0ffc
	.word	((__kuser_helper_end - __kuser_helper_start) >> 5)

	.globl	__kuser_helper_end
__kuser_helper_end:
