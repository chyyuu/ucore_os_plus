/*
 *  init.S 
 *  stack initialization
 *  supervisor mode
 *  jump to bootmain
 *
 *  author: YU Kuanlong / HE Alexis
 *  modified: Chen Yuheng
 *  date:   2012-3-21
 */

.text
.code 32
.align 0
.global _start
.extern __bss_start__
.extern __bss_end__

#include <board.h>

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

.equ _SVC_STACK, (TEXT_BASE+MEM_SIZE-4)
.equ _IRQ_STACK, (_SVC_STACK-0x400)
.equ _FIQ_STACK, (_IRQ_STACK-0x400)
.equ _ABT_STACK, (_FIQ_STACK-0x400)
.equ _UND_STACK, (_ABT_STACK-0x400)
.equ _SYS_STACK, (_UND_STACK-0x400)

_start: # start address at cpu reset is 0x0: jump to _vector_reset

# Exception vector table
__vector_table:
	b  _vector_reset
	ldr pc,_vector_undefined
	ldr pc,_vector_swi
	ldr pc,_vector_prefetch_abort
	ldr pc,_vector_data_abort
	ldr pc,_vector_reserved
	ldr pc,_vector_irq
	ldr pc,_vector_fiq

# To be replaced by OS with actual adresses of handlers
# .align 4
_vector_reset:	
	.word	__vector_reset
_vector_undefined:	
	.word	__vector_undefined
_vector_swi:	
	.word	__vector_swi
_vector_prefetch_abort:	
	.word	__vector_prefetch_abort
_vector_data_abort:	
	.word	__vector_data_abort
_vector_reserved:	
	.word	__vector_reserved
_vector_irq:	
	.word	__vector_irq
_vector_fiq:	
	.word	__vector_fiq 

# Stack inititialization - starts in SVC mode
__vector_reset:
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|IRQ_MOD)
	ldr sp,=_IRQ_STACK
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|FIQ_MOD)
	ldr sp,=_FIQ_STACK
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|ABT_MOD)
	ldr sp,=_ABT_STACK
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|UND_MOD)
	ldr sp,=_UND_STACK
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|SYS_MOD)
	ldr sp,=_SYS_STACK
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|SVC_MOD)
	ldr sp,=_SVC_STACK

__vector_undefined:
__vector_swi:
__vector_prefetch_abort:
__vector_data_abort:
__vector_reserved:
__vector_irq:
__vector_fiq:
	
_boot:
	bl bootmain
	b	_start

