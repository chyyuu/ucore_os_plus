		.text
		.global _start

		// This function should look like this in C:
		//     void _start (int argc, char* argv[])
_start:		
		l.bnf	umain
		l.nop
		l.nop

		// should never reach here
1:
		l.j 	1b
		l.nop
		l.nop