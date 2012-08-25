#include <or32/spr_defs.h>
#include <or32/or32_func.h>
#include <trap.h>
#include <board.h>
#include <gpio.h>
#include <mmu.h>
#include <memlayout.h>

.macro  exception_vector name org
	        .org \org
	        .p2align 8
	        .global  __exception_\name
	__exception_\name:
	        
	        l.j __exception_\name
	        l.nop
.endm

		.section .vectors, "ax"

// ==================== Reset Exception ====================
		.org 0x100 - 0x100

_reset:
		ENABLE_MMU (r3, r4)
		
		// Reset GPRs
		.equ 	sp, 0xc0004000            // [0x2000, 0x4000) is the stack for ucore kernel
		load32i	r0, 0
		load32i r1, sp
		l.or 	r2, r0, r0
		l.or 	r3, r0, r0
		l.or 	r4, r0, r0
		l.or 	r5, r0, r0
		l.or 	r6, r0, r0
		l.or 	r7, r0, r0
		l.or 	r8, r0, r0
		l.or 	r9, r0, r0
		l.or 	r10, r0, r0
		l.or 	r11, r0, r0
		l.or 	r12, r0, r0
		l.or 	r13, r0, r0
		l.or 	r14, r0, r0
		l.or 	r15, r0, r0
		l.or 	r16, r0, r0
		l.or 	r17, r0, r0
		l.or 	r18, r0, r0
		l.or 	r19, r0, r0
		l.or 	r20, r0, r0
		l.or 	r21, r0, r0
		l.or 	r22, r0, r0
		l.or 	r23, r0, r0
		l.or 	r24, r0, r0
		l.or 	r25, r0, r0
		l.or 	r26, r0, r0
		l.or 	r27, r0, r0
		l.or 	r28, r0, r0
		l.or 	r29, r0, r0
		l.or 	r30, r0, r0
		l.or 	r31, r0, r0

		// Jump to the init function (should never return)
		load32i r2, kern_init
		l.mtspr	r0, r2, SPR_EPCR_BASE
		l.mfspr	r2, r0, SPR_SR
		l.mtspr	r0, r2, SPR_ESR_BASE
		l.rfe

// ==================== Bus Error Exception ====================
		.org 0x200 - 0x100
		.global __bus_error
__bus_error:
		l.sw	GPR31_TMP(r0), r31
				
		load32i	r31, GPIO_PHYSICAL_BASE
		load32i	r30, 0x200
		//l.sw	GPIO_OUTPUT(r31), r30		

__asm_panic:
		load32i r31, __bus_error_exception
		l.j		__exception_handle
		l.nop

__exception_handle:
		l.sw	GPR1_TMP(r0), r1
		l.sw	GPR10_TMP(r0), r10
		l.sw	GPR13_TMP(r0), r13

		//check whether the exception happens in user mode.
		l.mfspr	r13, r0, SPR_ESR_BASE
		l.andi	r13, r13, SPR_SR_SM
		l.sfeq	r13, r0
		l.bnf	continue             // if SPR_SR_SM is set, we don't need to switch stack
		l.nop
		load32i	r13, pls_current
		tophys  (r10, r13)
		l.lwz	r13, 0(r10)          // this is what 'current' points to
		l.sfeq	r13, r0
		l.bf	continue             // 'current' points to NULL
		l.nop
		tophys  (r10, r13)
		l.lwz	r1, 12(r10)	  	     // now r1 points to the "top" of the kstack
		load32i	r10, KSTACKSIZE
		l.add	r1, r1, r10          // r1 finally points to the right position

continue:		
		// transform r1 to physical address.
		tophys (r13, r1)
		l.add	r1, r13, r0

		// move r1 down
		l.addi	r1, r1, -(TF_SIZE)

		// Save gprs that we'll use next.
		// r1: the kernel stack for exception handling
		// r4: EA of the exception
		// r10: current PCB
		// r12: temp register / syscall indicator (1 if from syscall)
		// r13: temp register
		// r31: handler of the exception
		l.lwz 	r13, GPR1_TMP(r0)
		l.sw	SP(r1), r13
		l.sw	GPR4(r1), r4
		l.lwz	r13, GPR10_TMP(r0)
		l.sw	GPR10(r1), r13
		l.sw	GPR12(r1), r12
		l.lwz	r13, GPR13_TMP(r0)
		l.sw	GPR13(r1), r13
		l.lwz	r13, GPR31_TMP(r0)
		l.sw	GPR31(r1), r13

		// Save exception registers
		// Note: we don't have fast context switch anywhere
		l.mfspr	r13, r0, SPR_EPCR_BASE
		l.sw	PC(r1), r13
		l.mfspr r13, r0, SPR_ESR_BASE
		l.sw	SR(r1), r13

		// load r4 with EA
		l.mfspr	r4, r0, SPR_EEAR_BASE
		// clear r12
		l.or	r12, r0, r0
		// turn MMU and cache on (we don't have it at present)
		l.ori	r13, r0, (SPR_SR_SM | SPR_SR_IME | SPR_SR_DME)
		l.mtspr r0, r13, SPR_ESR_BASE

		// transform r1 back to virtual address
		tovirt 	(r13, r1)
		l.add	r1, r13, r0

		// hacking epcr...
		l.mtspr	r0, r31, SPR_EPCR_BASE
		l.rfe

// ==================== Data Page Fault Exception ====================
		.org 0x300 - 0x100
		
		load32i	r31, GPIO_PHYSICAL_BASE
		load32i	r30, 0x300
		//l.sw	GPIO_OUTPUT(r31), r30

		.global __data_page_fault
__data_page_fault:
		l.sw	GPR31_TMP(r0), r31
		load32i	r31, __dpage_fault_exception
		l.j		__exception_handle
		l.nop

// ==================== Insn Page Fault Exception ====================
		.org 0x400 - 0x100
		
		load32i	r31, GPIO_PHYSICAL_BASE
		load32i	r30, 0x400
		//l.sw	GPIO_OUTPUT(r31), r30

		.global __insn_page_fault
__insn_page_fault:		
		l.sw	GPR31_TMP(r0), r31
		load32i	r31, __ipage_fault_exception
		l.j		__exception_handle
		l.nop

// ==================== Tick Timer Exception ====================
		.org 0x500 - 0x100
		
		load32i	r31, GPIO_PHYSICAL_BASE
		load32i	r30, 0x500
		//l.sw	GPIO_OUTPUT(r31), r30

		.global __tick_timer
__tick_timer:
		l.sw	GPR31_TMP(r0), r31
		load32i r31, __tick_timer_exception
		l.j		__exception_handle
		l.nop

// ==================== Unaligned Access Exception ====================
		.org 0x600 - 0x100
		.global __unaligned_access
__unaligned_access:
		
		load32i	r31, GPIO_PHYSICAL_BASE
		load32i	r30, 0x600
		l.sw	GPIO_OUTPUT(r31), r30
		
		l.j		__asm_panic
		l.nop		

	exception_vector        illegal_instruction     0x700 - 0x100

// ==================== External Interrupt Exception ====================
		.org 0x800 - 0x100

        .global  __external_IRQ
__external_IRQ: 
		l.sw	GPR31_TMP(r0), r31
		
		load32i	r31, GPIO_PHYSICAL_BASE
		load32i	r30, 0x800
		//l.sw	GPIO_OUTPUT(r31), r30
		
		load32i r31, __external_exception
		l.j		__exception_handle
		l.nop

// ==================== Boot-Time Data TLB Miss Exception ====================
		.org 0x900 - 0x100
		
		.global __dtlb_miss
__dtlb_miss:
		// This will be modified in page_init to __post_boot_dtlb_miss
		l.nop
		l.nop
		l.j		__boot_dtlb_miss
		l.nop
		l.nop

__boot_dtlb_miss:
		// This handler is only used before any page table is built up
		
/* attributes set for DTLB_MR register: - (0) sets V (valid) bit
 */
#define DTLB_MR_ATTR 0x00000001

/* attributes set for DTLB_TR register: - (2) sets CI (cache inhibit) bit,
 *			                            - (4) sets A (access) bit, 
 *                                      - (5) sets D (dirty) bit,
 *                                      - (8) sets SRE (superuser read) bit
 *                                      - (9) sets SWE (superuser write) bit
 */
#define DTLB_TR_ATTR 0x00000332

#define VPN_MASK 0xffffe000
#define PPN_MASK 0xffffe000
		
		l.sw	GPR2_TMP(r0), r2
		l.sw	GPR3_TMP(r0), r3
		l.sw	GPR4_TMP(r0), r4
		l.sw	GPR5_TMP(r0), r5
		l.sw	GPR6_TMP(r0), r6
		
		load32i	r3, GPIO_PHYSICAL_BASE
		load32i	r4, 0x900
		//l.sw	GPIO_OUTPUT(r3), r4

		// GPR usage:
		//     r2: set number
		//	   r3: corresponding physical address
		//     r4: offending EA
		//	   r5: the Data TLB Matching Register / Translation Register entry

		l.mfspr	r4, r0, SPR_EEAR_BASE
		load32i r3, KERNBASE
		l.sfltu r4, r3                          // set flag if r4 < KERNBASE
		l.bf	1f	
		l.nop
		load32i	r3, MMIO_BASE                   // the space above MMIO_BASE is used for mmio
		l.sfgeu	r4, r3
		l.bf	3f
		l.nop
		tophys 	(r3, r4)                        // r3 now holds the physical address
		l.j		2f
		l.nop
3:		// EA >= 0xF0000000
		l.slli	r6, r4, 4
		l.and	r3, r6, r3
		l.j		2f
		l.nop
1:      // EA < 0xC0000000
		l.add	r3, r0, r4
2:		

		// This segment of code may be used (copied) by post-boot tlb miss handler
		// when an access to disks is required.
		// Arguments: r3: physical address of the page
		//            r4: virtual address of the page
		
		// Carry out boot immediate translation
		l.srli	r6, r4, PGSHIFT
		l.addi  r2, r0, (NR_SETS - 1)  // r2 stores the mask for set number
		l.and	r2, r6, r2             // now r2 holds the set number

		// Form the Matching Register entry
		load32i r6, VPN_MASK
		l.and	r5, r4, r6
		l.ori	r5, r5, DTLB_MR_ATTR
		// Store to the corresponding TLB MR
		l.mtspr	r2, r5, SPR_DTLBMR_BASE(0)

		// Form the Translation Register entry
		load32i	r6, PPN_MASK
		l.and	r5, r3, r6
		l.ori	r5, r5, DTLB_TR_ATTR
		// Store to the corresponding TLB TR
		l.mtspr r2, r5, SPR_DTLBTR_BASE(0)

		// Everything is done normally
		l.lwz	r2, GPR2_TMP(r0)
		l.lwz	r3, GPR3_TMP(r0)
		l.lwz	r4, GPR4_TMP(r0)
		l.lwz	r5, GPR5_TMP(r0)
		l.lwz	r6, GPR6_TMP(r0)
		l.rfe

__exit_with_no_translation:
		l.lwz	r2, GPR2_TMP(r0)
		l.lwz	r3, GPR3_TMP(r0)
		l.lwz	r4, GPR4_TMP(r0)
		l.lwz	r5, GPR5_TMP(r0)
		l.lwz	r6, GPR6_TMP(r0)
		l.j		__asm_panic

// ==================== Boot-Time Insn TLB Miss Exception ====================
		.org 0xa00 - 0x100
		
		.global __itlb_miss
__itlb_miss:		
		// This will be modified in page_init to __post_boot_itlb_miss
		l.nop
		l.nop
		l.j		__boot_itlb_miss
		l.nop
		l.nop
		
__boot_itlb_miss:
		// This handler is only used before any page table is built up

/* attributes set for ITLB_MR register: - sets V (valid) bit, 
 */
#define ITLB_MR_ATTR 0x00000001

/* attributes set for ITLB_TR register: - sets A (access) bit, 
 *                                      - sets SXE (superuser execute) bit
 */
#define ITLB_TR_ATTR 0x00000050

#define VPN_MASK 0xffffe000
#define PPN_MASK 0xffffe000
		
		l.sw	GPR2_TMP(r0), r2
		l.sw	GPR3_TMP(r0), r3
		l.sw	GPR4_TMP(r0), r4
		l.sw	GPR5_TMP(r0), r5
		l.sw	GPR6_TMP(r0), r6

		load32i	r3, GPIO_PHYSICAL_BASE
		load32i	r4, 0xa00
		//l.sw	GPIO_OUTPUT(r3), r4

		// GPR usage:
		//     r2: set number
		//	   r3: corresponding physical address
		//     r4: offending EA
		//	   r5: the Data TLB Matching Register / Translation Register entry

		// Ensure that the physical address is valid, i.e. in [0x0, RAM_SIZE)
		l.mfspr	r4, r0, SPR_EEAR_BASE
		load32i r3, KERNBASE
		l.sfltu r4, r3                          // set flag if r4 < KERNBASE
		l.bf	1f	
		l.nop
		tophys 	(r3, r4)                        // r3 now holds the physical address
		l.j		2f
		l.nop
1:
		l.add	r3, r0, r4
2:		
		load32i	r6, RAM_SIZE
		l.sfgeu r3, r6
		//l.bf	__exit_with_no_translation      // use the dtlb miss handler's error handling
		l.nop

		// Carry out boot immediate translation
		l.srli	r6, r4, PGSHIFT
		l.addi  r2, r0, (NR_SETS - 1)  // r2 stores the mask for set number
		l.and	r2, r6, r2             // now r2 holds the set number

		// Form the Matching Register entry
		load32i r6, VPN_MASK
		l.and	r5, r4, r6
		l.ori	r5, r5, ITLB_MR_ATTR
		// Store to the corresponding TLB MR
		l.mtspr	r2, r5, SPR_ITLBMR_BASE(0)

		// Form the Translation Register entry
		load32i	r6, PPN_MASK
		l.and	r5, r3, r6
		l.ori	r5, r5, ITLB_TR_ATTR
		// Store to the corresponding TLB TR
		l.mtspr r2, r5, SPR_ITLBTR_BASE(0)

		// Everything is done normally
		l.lwz	r2, GPR2_TMP(r0)
		l.lwz	r3, GPR3_TMP(r0)
		l.lwz	r4, GPR4_TMP(r0)
		l.lwz	r5, GPR5_TMP(r0)
		l.lwz	r6, GPR6_TMP(r0)
		l.rfe

//==================== Syscall 'Exception' ====================
		.org 0xc00 - 0x100
		.global __syscall
__syscall:
		l.sw	GPR31_TMP(r0), r31
		load32i r31, __syscall_exception
		l.j		__exception_handle
		l.nop

//==================== Floating Point Exception ====================
		.org 0xe00 - 0x100
		.global __floating_point
__floating_point:
		l.sw	GPR31_TMP(r0), r31
		load32i r31, __floating_point_exception
		l.j		__exception_handle
		l.nop


//==================== Post-Boot Data TLB Exception Handler ====================
		.org 0x1c00 - 0x200
		
#define DTLB_POST_BOOT_TR_ATTR (SPR_DTLBTR_A | SPR_DTLBTR_D | SPR_DTLBTR_URE | SPR_DTLBTR_UWE | SPR_DTLBTR_SRE | SPR_DTLBTR_SWE)
		
		.global __post_boot_dtlb_miss
__post_boot_dtlb_miss:
		l.sw	GPR2_TMP(r0), r2
		l.sw 	GPR3_TMP(r0), r3
		l.sw	GPR4_TMP(r0), r4
		l.sw	GPR5_TMP(r0), r5
		l.sw	GPR6_TMP(r0), r6
		
		l.mfspr	r2, r0, SPR_EEAR_BASE

		// Check whether its a mmio access (whose EA's highest 4 bits should be F)
		load32i	r3, 0xF0000000
		l.and	r4, r2, r3
		l.sfeq	r3, r4
		l.bnf	page_table_check
		l.nop

		// Mmm... do the map myself, using the boot-time loader
		// Arguments: r3: physical address of the page
		//                which should be like 0xY0nnnnnn
		//            r4: virtual address of the page
		//                which should be like 0xfYnnnnnn

		// Prepare the physical address
		// step 1: get the 'Y' bit
		l.srli	r4, r2, 24
		l.andi	r4, r4, 0xF
		l.slli	r4, r4, 28
		// step 2: fill the lower 24 bits
		load32i	r5, 0x00FFFFFF
		l.and	r5, r2, r5
		l.or	r3, r4, r5
		
		// Prepare the virtual address
		l.add	r4, r0, r2
		
		// Carry out boot immediate translation
		l.srli	r6, r4, PGSHIFT
		l.addi  r2, r0, (NR_SETS - 1)  // r2 stores the mask for set number
		l.and	r2, r6, r2             // now r2 holds the set number

		// Form the Matching Register entry
		load32i r6, VPN_MASK
		l.and	r5, r4, r6
		l.ori	r5, r5, DTLB_MR_ATTR
		// Store to the corresponding TLB MR
		l.mtspr	r2, r5, SPR_DTLBMR_BASE(0)

		// Form the Translation Register entry
		load32i	r6, PPN_MASK
		l.and	r5, r3, r6
		l.ori	r5, r5, DTLB_POST_BOOT_TR_ATTR
		// Store to the corresponding TLB TR
		l.mtspr r2, r5, SPR_DTLBTR_BASE(0)

		l.j		recover
		l.nop


		// General register usage:
		//    r2: EA
		//    r3: base address of pgdir (can be temp)
		//    r4: entry index (can be temp)
		//    r5: entry content (pde/pte)
page_table_check:
		load32i	r4, current_pgdir_pa
		tophys 	(r3, r4)
		l.lwz	r3, 0(r3)
		l.srli	r4, r2, PDXSHIFT
		l.slli	r4, r4, 2                  // 4 bytes per entry
		l.add	r5, r3, r4
		l.lwz	r5, 0(r5)
		l.sfne	r5, r0
		l.bnf	d_pmd_none
		l.nop

		// calculate 2nd level pgdir base address
		load32i	r3, ~(PGSIZE-1)
		l.and	r3, r5, r3

		// calculate index in 2nd level pgdir
		l.srli	r4, r2, PTXSHIFT
		l.andi	r4, r4, (NPTEENTRY - 1)
		l.slli	r4, r4, 2
		l.add	r5, r3, r4
		l.lwz	r5, 0(r5)                  // now (r5) = pte

		// check whether the page is present
		l.andi 	r4, r5, 1
		l.sfne	r4, r0
		l.bnf	d_pte_none

		// now everything is ready, let's fill MR and TR
		// Note: if the access violates authorization, a page fault will raised after we return.

		// calculate tlb set number
		l.srli	r4, r2, PTXSHIFT
		l.andi	r4, r4, (NR_SETS - 1)
		// fill Translation Register
		load32i	r3, (~(PGSIZE-1)) | PTE_SPR_W | PTE_SPR_R | PTE_USER_W | PTE_USER_R
		l.and	r3, r5, r3
		// play a trick on the entry
		l.andi	r6, r5, PTE_A
		l.sfeq	r6, r0
		l.bnf	10f
		l.nop
		// unset read access bits
		load32i	r6, ~(PTE_SPR_R | PTE_USER_R)
		l.and	r3, r3, r6
10:
		l.andi	r6, r5, PTE_D
		l.sfeq	r6, r0
		l.bnf	11f
		l.nop
		// unset write access bits
		load32i	r6, ~(PTE_SPR_W | PTE_USER_W)
		l.and	r3, r3, r6
11:
		// finally load the entry to translation register
		l.mtspr	r4, r3, SPR_DTLBTR_BASE(0)
		
		// fill Matching Register
		load32i	r3, ~(PGSIZE - 1)
		l.and	r3, r2, r3
		l.ori	r3, r3, 1                  // Set the valid bit
		l.mtspr	r4, r3, SPR_DTLBMR_BASE(0)

		// recover
recover:		
		l.lwz	r2, GPR2_TMP (r0)
		l.lwz	r3, GPR3_TMP (r0)
		l.lwz	r4, GPR4_TMP (r0)
		l.lwz	r5, GPR5_TMP (r0)
		l.lwz	r6, GPR6_TMP (r0)
		l.rfe
		
d_pmd_none:
d_pte_none:
		// let the page fault handler deal with this
		l.lwz	r2, GPR2_TMP (r0)
		l.lwz	r3, GPR3_TMP (r0)
		l.lwz	r4, GPR4_TMP (r0)
		l.lwz	r5, GPR5_TMP (r0)
		l.nop
		l.j		__data_page_fault
		l.nop
		l.nop

//==================== Post-Boot Insn TLB Exception Handler ====================
		.org 0x1e00 - 0x200
		
		.global __post_boot_itlb_miss
__post_boot_itlb_miss:
		l.sw	GPR2_TMP(r0), r2
		l.sw 	GPR3_TMP(r0), r3
		l.sw	GPR4_TMP(r0), r4
		l.sw	GPR5_TMP(r0), r5

		// General register usage:
		//    r2: EA
		//    r3: base address of pgdir (can be temp)
		//    r4: entry index (can be temp)
		//    r5: entry content (pde/pte)
		l.mfspr	r2, r0, SPR_EEAR_BASE
		load32i	r4, current_pgdir_pa
		tophys 	(r3, r4)
		l.lwz	r3, 0(r3)
		l.srli	r4, r2, PDXSHIFT
		l.slli	r4, r4, 2                  // 4 bytes per entry
		l.add	r5, r3, r4
		l.lwz	r5, 0(r5)
		l.sfne	r5, r0
		l.bnf	i_pmd_none
		l.nop

		// calculate 2nd level pgdir base address
		load32i	r3, ~(PGSIZE-1)
		l.and	r3, r5, r3

		// calculate index in 2nd level pgdir
		l.srli	r4, r2, PTXSHIFT
		l.andi	r4, r4, (NPTEENTRY - 1)
		l.slli	r4, r4, 2
		l.add	r5, r3, r4
		l.lwz	r5, 0(r5)                  // now (r5) = pte

		// check whether the page is present
		l.andi 	r4, r5, 1
		l.sfne	r4, r0
		l.bnf	i_pte_none
		l.nop

		// now everything is ready, let's fill MR and TR
		// Note: if the access violates authorization, a page fault will be raised after we return.

		// calculate tlb set number
		l.srli	r4, r2, PTXSHIFT
		l.andi	r4, r4, (NR_SETS - 1)
		// fill Translation Register
		load32i	r3, (~(PGSIZE - 1))
		l.and	r3, r5, r3
		l.ori	r3, r3, (SPR_ITLBTR_SXE | SPR_ITLBTR_UXE)
		l.mtspr	r4, r3, SPR_ITLBTR_BASE(0)
		// fill Matching Register
		load32i	r3, ~(PGSIZE - 1)
		l.and	r3, r2, r3
		l.ori	r3, r3, SPR_ITLBMR_V
		l.mtspr	r4, r3, SPR_ITLBMR_BASE(0)

		// recover
		l.lwz	r2, GPR2_TMP (r0)
		l.lwz	r3, GPR3_TMP (r0)
		l.lwz	r4, GPR4_TMP (r0)
		l.lwz	r5, GPR5_TMP (r0)
		l.rfe
		
i_pmd_none:
i_pte_none:
		// let the page fault handler deal with this
		l.lwz	r2, GPR2_TMP (r0)
		l.lwz	r3, GPR3_TMP (r0)
		l.lwz	r4, GPR4_TMP (r0)
		l.lwz	r5, GPR5_TMP (r0)
		l.nop
		l.j		__insn_page_fault
		l.nop
		l.nop
