/*
 * Assembly functions for software multiplication and devision.
 */

#define ENTRY(symbol)	\
	.align 4	;\
	.global symbol	;\
symbol:			

ENTRY(__mulsi3)
	l.addi r11,r0,0x0
	l.sfne r3,r11
	l.bnf 3f
	l.ori r5,r3,0x0
	l.addi r6,r0,0x0
1:
	l.andi r3,r5,0x1
	l.sfeq r3,r6
	l.bf 2f
	l.srli r5,r5,0x1
	l.add r11,r11,r4
2:
	l.sfne r5,r6
	l.bf 1b
	l.slli r4,r4,0x1
3:
	l.jr r9
	l.nop 0x0

ENTRY(__muldi3)
	l.andi			r7,r4,0xffff
	l.srli			r8,r4,0x10
	l.andi			r11,r6,0xffff
	l.srli			r12,r6,0x10
	l.mul			r13,r11,r7
	l.mul			r11,r11,r8
	l.mul			r7,r12,r7
	l.srli			r15,r13,0x10
	l.add			r7,r11,r7
	l.sw			-4(r1),r2
	l.add			r7,r7,r15
	l.addi			r1,r1,-4
	l.sfleu			r11,r7
	l.bf			1f
	l.mul			r8,r12,r8
	l.movhi			r2,1
	l.add			r8,r8,r2
1:
	l.mul			r6,r6,r3
	l.mul			r4,r4,r5
	l.srli			r3,r7,0x10
	l.slli			r7,r7,0x10
	l.andi			r13,r13,0xffff
	l.add			r8,r8,r3
	l.add			r11,r4,r6
	l.addi			r1,r1,0x4
	l.add			r12,r7,r13
	l.add			r11,r11,r8
	l.jr			r9
	l.lwz			r2, -4(r1)
	

ENTRY(__udivsi3)
	l.addi          r1,r1,-4
        l.sw            0(r1),r9
        l.addi          r11,r0,0
        l.addi          r8,r4,0
        l.addi          r5,r3,0
        l.sfne          r8,r11
        l.bnf           4f
        l.addi          r7,r0,0
        l.sfgtu         r8,r5
        l.bf            5f
        l.sfeq          r8,r5
        l.bf            6f
        l.sfltu         r11,r8
        l.bnf           2f
        l.addi          r13,r0,32
        l.movhi         r9,hi(0x80000000)
        l.addi          r6,r0,-1
1:
        l.and           r3,r5,r9
        l.slli          r4,r7,1
        l.addi          r15,r5,0
        l.srli          r3,r3,31
        l.add           r13,r13,r6
        l.or            r7,r4,r3
        l.sfltu         r7,r8
        l.bf            1b
        l.slli          r5,r5,1
2:
        l.srli          r7,r7,1
        l.addi          r13,r13,1
        l.addi          r9,r0,0
        l.sfltu         r9,r13
        l.bnf           4f
        l.addi          r5,r15,0
        l.movhi         r15,hi(0x80000000)
        l.addi          r17,r0,0
3:
        l.and           r3,r5,r15
        l.slli          r4,r7,1
        l.srli          r3,r3,31
        l.or            r7,r4,r3
        l.sub           r6,r7,r8
        l.and           r3,r6,r15
        l.srli          r3,r3,31
        l.addi          r4,r0,0
        l.sfne          r3,r4
        l.bf            1f
        l.slli          r3,r11,1
        l.addi          r4,r0,1
1:
        l.slli          r5,r5,1
        l.sfne          r4,r17
        l.bnf           2f
        l.or            r11,r3,r4
        l.addi          r7,r6,0
2:
        l.addi          r9,r9,1
        l.sfltu         r9,r13
        l.bf            3b
        l.nop           0
	l.j		4f
	l.nop           0
6:	
	l.j		4f
	l.addi		r11,r0,1
5:
	l.addi		r7,r5,0
4:
        l.lwz           r9,0(r1)
        l.jr            r9
        l.addi          r1,r1,4

ENTRY(__divsi3)
	l.addi          r1,r1,-8
        l.sw            0(r1),r9
        l.sw            4(r1),r14
        l.addi          r5,r3,0
        l.addi          r14,r0,0
        l.sflts         r5,r0
        l.bnf           1f
        l.addi          r3,r0,0
        l.addi          r14,r0,1
        l.sub           r5,r0,r5
1:
        l.sflts         r4,r0
        l.bnf           1f
        l.nop           0
        l.addi          r14,r14,1
        l.sub           r4,r0,r4
1:
        l.jal           __udivsi3
        l.addi          r3,r5,0
        l.sfeqi         r14,1
        l.bnf           1f
        l.nop           0
        l.sub           r11,r0,r11
1:
        l.lwz           r9,0(r1)
        l.lwz           r14,4(r1)
        l.jr            r9
        l.addi          r1,r1,8

ENTRY(__umodsi3)
	l.addi          r1,r1,-4
	l.sw            0(r1),r9
	l.jal           __udivsi3
	l.nop           0
	l.addi		r11,r7,0
	l.lwz           r9,0(r1)
	l.jr            r9
	l.addi          r1,r1,4
	
ENTRY(__modsi3)
        l.addi          r1,r1,-8
        l.sw            0(r1),r9
        l.sw            4(r1),r14
        l.addi          r14,r0,0
        l.sflts         r3,r0
        l.bnf           1f
        l.nop           0
        l.addi          r14,r0,1
        l.sub           r3,r0,r3
1:
        l.sflts         r4,r0
        l.bnf           1f
        l.nop           0
        l.sub           r4,r0,r4
1:
        l.jal           __udivsi3
        l.nop           0
        l.sfeqi         r14,1
        l.bnf           1f
        l.addi		r11,r7,0
        l.sub           r11,r0,r11
1:
        l.lwz           r9,0(r1)
        l.lwz           r14,4(r1)
        l.jr            r9
        l.addi          r1,r1,8	
