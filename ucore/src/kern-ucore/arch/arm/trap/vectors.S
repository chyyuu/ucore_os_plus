#include <board.h>

.text
.global __vector_reset
.global __vector_undefined
.global __vector_swi
.global __vector_prefetch_abort
.global __vector_data_abort
.global __vector_reserved
.global __vector_irq
.global __vector_fiq
.global __alltraps
.global __vector_table
.global __vector_table_end

.align 4

__vector_table:
	b  _vector_reset
	ldr pc,_vector_undefined
	ldr pc,_vector_swi
	ldr pc,_vector_prefetch_abort
	ldr pc,_vector_data_abort
	ldr pc,_vector_reserved
	ldr pc,_vector_irq
	ldr pc,_vector_fiq

# Exception vector table has been set by bootasm.S
# We replace only the adresses of the handlers
# We decide that a reset should recopy the kernel 
# so we don't change the adress for __vector_reset
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

__vector_table_end:


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

# Reset - initialize stack for each cpu mode
__vector_reset:
	msr cpsr_c,#(DISABLE_IRQ|DISABLE_FIQ|SVC_MOD)
	ldr sp,=_SVC_STACK
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

.globl irq_proc
# Exception modes
__vector_undefined:
	b real_vector_und
__vector_swi:
	b __switrap
__vector_prefetch_abort:
	b real_vector_pabt
__vector_data_abort:
	b real_vector_dabt
__vector_reserved:
	b __vector_reserved
__vector_irq:
	b real_vector_irq
__vector_fiq:
	b __vector_fiq
