		.text
		.global _start

		// This function should look like this in C:
		//     void _start (int argc, char* argv[])
_start:		
    ldr r0, [sp]
    #ldr r1, [sp, #+4]
    add r1, sp, #4
		bl	umain
		nop

		// should never reach here
_dead:
		b _dead
		nop
