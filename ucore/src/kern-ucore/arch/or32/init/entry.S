#include <trap.h>
#include <gpio.h>
#include <or32/spr_defs.h>
#include <or32/or32_func.h>

		.text
		// save the rest of GPRs
__save_state:
		l.sw	GPR2(r1), r2
		l.sw	GPR3(r1), r3
		l.sw	GPR5(r1), r5
		l.sw	GPR6(r1), r6
		l.sw	GPR7(r1), r7
		l.sw	GPR8(r1), r8
		l.sw	GPR11(r1), r11
		l.sw	GPR14(r1), r14
		l.sw	GPR15(r1), r15
		l.sw	GPR16(r1), r16
		l.sw	GPR17(r1), r17
		l.sw	GPR18(r1), r18
		l.sw	GPR19(r1), r19
		l.sw	GPR20(r1), r20
		l.sw	GPR21(r1), r21
		l.sw	GPR22(r1), r22
		l.sw	GPR23(r1), r23
		l.sw	GPR24(r1), r24
		l.sw	GPR25(r1), r25
		l.sw	GPR26(r1), r26
		l.sw	GPR27(r1), r27
		l.sw	GPR28(r1), r28
		l.sw	GPR29(r1), r29
		l.sw	GPR30(r1), r30
		l.sw	RESULT(r1), r0
		l.jr	r9
		l.nop

		// Restore GPRs and return from exception
__restore_state:
		DISABLE_INTERRUPTS (r3, r4)
		l.lwz	r3, PC(r1)
		l.mtspr	r0, r3, SPR_EPCR_BASE
		l.lwz	r3, SR(r1)
		l.mtspr r0, r3, SPR_ESR_BASE
		l.lwz	r2, GPR2(r1)
		l.lwz	r3, GPR3(r1)
		l.lwz	r4, GPR4(r1)
		l.lwz	r5, GPR5(r1)
		l.lwz	r6, GPR6(r1)
		l.lwz	r7, GPR7(r1)
		l.lwz	r8, GPR8(r1)
		l.lwz	r9, GPR9(r1)
		l.lwz	r10, GPR10(r1)
		l.lwz	r11, GPR11(r1)
		l.lwz	r12, GPR12(r1)
		l.lwz	r13, GPR13(r1)
		l.lwz	r14, GPR14(r1)
		l.lwz	r15, GPR15(r1)
		l.lwz	r16, GPR16(r1)
		l.lwz	r17, GPR17(r1)
		l.lwz	r18, GPR18(r1)
		l.lwz	r19, GPR19(r1)
		l.lwz	r20, GPR20(r1)
		l.lwz	r21, GPR21(r1)
		l.lwz	r22, GPR22(r1)
		l.lwz	r23, GPR23(r1)
		l.lwz	r24, GPR24(r1)
		l.lwz	r25, GPR25(r1)
		l.lwz	r26, GPR26(r1)
		l.lwz	r27, GPR27(r1)
		l.lwz	r28, GPR28(r1)
		l.lwz	r29, GPR29(r1)
		l.lwz	r30, GPR30(r1)
		l.lwz	r31, GPR31(r1)
		l.lwz	r1, SP(r1)
		l.rfe

__ret_from_exception:
		// TODO: we should judge whether we're going to return to user mode or not
		l.j		__restore_state
		l.nop

// ==================== Bus Error Exception (0x200) ====================
// This is used as kernel panic
		.global __bus_error_exception
__bus_error_exception:
		// Store the rest of GPRs
		l.sw	GPR9(r1), r9
		l.jal	__save_state
		l.nop

		l.add	r3, r0, r1
		l.ori	r5, r0, 0x200

		// Call the main exception handler
		//l.jal	bus_error_exception
		l.jal	trap
		l.nop

		// truly return from exception (will we?)
		l.j		__ret_from_exception
		l.nop


// ==================== Data Page Fault Exception (0x300) ====================
		.global __dpage_fault_exception
__dpage_fault_exception:		
		// Store the rest of GPRs
		l.sw	GPR9(r1), r9
		l.jal	__save_state
		l.nop

		// Set up arguments for do_page_fault:
		//     r3: trapframe pointer
		//	   r4: offending EA
		//     r5: vector number of the current handler
		//     r6: whether it is a write access

		// Set up r3
		l.add	r3, r0, r1

		// r4 has already been set when we get here.

		// Set up r5
		l.ori	r5, r0, 0x300

		// Set up r6
		// Note: or1200 doesn't set DSX in SR
		l.lwz	r6, PC(r3)
		l.lwz	r6, 0(r6)
		l.srli	r6, r6, 26           // the first 6 bits are opcode for insns
		l.sfeqi	r6, 0                // l.j
		l.bf	8f
		l.sfeqi r6, 1				 // l.jal
		l.bf	8f
		l.sfeqi r6, 3 				 // l.bnf
		l.bf	8f
		l.sfeqi r6, 4				 // l.bf
		l.bf	8f
		l.sfeqi r6, 0x11			 // l.jr
		l.bf	8f
		l.sfeqi	r6, 0x12			 // l.jalr
		l.bf	8f
		l.nop

		l.j		9f
		l.nop

8:	    // EPCR points to a branch insn, i.e. the offending insn is in the delay slot
		l.lwz	r6, PC(r3)
		l.addi	r6, r6, 4
		l.lwz	r6, 0(r6)
		l.srli	r6, r6, 26
		
9:		// we have the correct opcode finally
		// store insns' opcode are in the range [0x34, 0x37]
		l.sfgeui r6, 0x34
		l.bnf	1f
		l.sfleui r6, 0x37
		l.bnf	1f
		l.ori	r6, r0, 1            // write access
		l.j		2f
		l.nop
1:
		l.add	r6, r0, r0           // read access
2:		
		// Now everything is done. Call the main exception handler
		//l.jal	do_page_fault
		l.jal	trap
		l.nop

		// truly return from exception
		l.j		__ret_from_exception
		l.nop

// ==================== Insn Page Fault Exception (0x400) ====================
		.global __ipage_fault_exception
__ipage_fault_exception:		
		// Store the rest of GPRs
		l.sw	GPR9(r1), r9
		l.jal	__save_state
		l.nop

		//load32i	r3, GPIO_BASE
		//l.sw	GPIO_OUTPUT(r3), r4
		
		// Set up r3
		l.add	r3, r0, r1

		// r4 has already been set when we get here.

		// Set up r5
		l.ori	r5, r0, 0x400

		// Set up r6
		l.addi	r6, r0, 2
		
		// Call the main exception handler
		//l.jal	do_page_fault
		l.jal	trap
		l.nop

		// truly return from exception
		l.j		__ret_from_exception
		l.nop

// ==================== Tick Timer Exception (0x500) ====================
		.global __tick_timer_exception
__tick_timer_exception:
		// Store the rest of GPRs
		l.sw	GPR9(r1), r9
		l.jal	__save_state
		l.nop

		l.add	r3, r0, r1
		l.ori	r5, r0, 0x500

		// Call the main exception handler
		//l.jal	tick_timer_exception
		l.jal	trap
		l.nop

		// truly return from exception
		l.j		__ret_from_exception
		l.nop

// ==================== External Exception (0x800) ====================
		.global __external_exception
__external_exception:
		// Store the rest of GPRs
		l.sw	GPR9(r1), r9
		l.jal	__save_state
		l.nop

		l.add	r3, r0, r1
		l.ori	r5, r0, 0x800
		// Call the main exception handler
		//l.jal	external_exception
		l.jal	trap
		l.nop

		// truly return from exception
		l.j		__ret_from_exception
		l.nop
		
// ==================== Syscall Exception (0xC00) ====================
		.global __syscall_exception
__syscall_exception:
		// Store the rest of GPRs
		l.sw	GPR9(r1), r9
		l.jal	__save_state
		l.nop

		l.add	r3, r0, r1
		l.ori	r5, r0, 0xC00
		// Call the main exception handler
		//l.jal	syscall_exception
		l.jal	trap
		l.nop

		// truly return from exception (will we?)
		l.j		__ret_from_exception
		l.nop

// ==================== Floating Point Exception (0xD00) ====================
		.global __floating_point_exception
__floating_point_exception:
		// Store the rest of GPRs
		l.sw	GPR9(r1), r9
		l.jal	__save_state
		l.nop

		l.add	r3, r0, r1
		l.ori	r5, r0, 0xD00
		// Call the main exception handler
		//l.jal	syscall_exception
		l.jal	trap
		l.nop

		// truly return from exception (will we?)
		l.j		__ret_from_exception
		l.nop

		
// ==================== return from fork ====================
		.global forkrets
		// void forkrets (struct trapframe *tf);
forkrets:
		l.add	r1, r3, r0
		l.j		__ret_from_exception
		l.nop