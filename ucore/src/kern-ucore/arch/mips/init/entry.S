
#include <asm/mipsregs.h>

.global kernel_entry
.extern kern_init

.extern edata
.extern end

.set noreorder

.section .startup

#define KSTACKSIZE (4096*2)

kernel_entry:
  b reset
  nop

	.align 4
reset:
  /* from u-boot */
#ifdef MACH_QEMU
	/* Clear watch registers */
	mtc0	$zero, CP0_WATCHLO
	mtc0	$zero, CP0_WATCHHI
#endif

	/* WP(Watch Pending), SW0/1 should be cleared */
	mtc0	$zero, CP0_CAUSE
  /* clear SR(ERL), which is 1 after reset */
	mtc0	$zero, CP0_STATUS

  jal 1f
  nop
  .word _gp
1:
  lw $gp, 0($ra)
  la $sp, bootstacktop
#setup ram exception
  la   $t0, __exception_vector
#TODO
  mtc0 $t0, $15, 1 

  mfc0 $t0, CP0_STATUS
  li   $t1, ~ST0_BEV
  and  $t0, $t0, $t1
  mtc0 $t0, CP0_STATUS

#zero bss
  la $t0, edata
  la $t1, end
  
2:
  sw $zero, 0($t0)
  addiu $t0, $t0, 4
  slt $t3, $t0, $t1
  bgtz $t3, 2b
  nop 

  addiu $sp, $sp, -16
  la $t9, kern_init 
  jal $t9
  nop
#never here
  bgez $zero, .
  nop


# bfc00280: R4000 xtlbmiss vector
romExcHandle_280:
  b romExcHandle_280
  nop

# bfc00300: R4000 cache vector
romExcHandle_300:
  b romExcHandle_300
  nop


.section .data
    .global bootstack
bootstack:
    .space KSTACKSIZE
    .global bootstacktop
bootstacktop:
    .space 32


