.text
.globl switch_to
switch_to:                      # switch_to(from, to)
	# TODO: select relevant registers to keep

    # save from's registers
	# in the structure pointed by r0
  stmia r0!, {r4-r10, fp, ip, sp, lr}
  mrs   r4, cpsr
  stmia r0!, {r4}

    # restore to's registers
	# in the structure pointed by r1
  ldr   r4, [r1, #+44]
  msr   cpsr, r4

	ldmia r1!, {r4-r10, fp, ip, sp, lr}
	
	# return
	mov pc, lr

