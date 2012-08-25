		.text
		.global switch_to
		// switch_to (struct context *from, struct context *to);
switch_to:
		l.sw	0(r3), r1
		l.sw	4(r3), r2
		l.sw	16(r3), r5
		l.sw	20(r3), r6
		l.sw	24(r3), r7
		l.sw	28(r3), r8
		l.sw	32(r3), r9
		l.sw	36(r3), r10
		l.sw	40(r3), r11
		l.sw	44(r3), r12
		l.sw	48(r3), r13
		l.sw	52(r3), r14
		l.sw	56(r3), r15
		l.sw	60(r3), r16
		l.sw	64(r3), r17
		l.sw	68(r3), r18
		l.sw	72(r3), r19
		l.sw	76(r3), r20
		l.sw	80(r3), r21
		l.sw	84(r3), r22
		l.sw	88(r3), r23
		l.sw	92(r3), r24
		l.sw	96(r3), r25
		l.sw	100(r3), r26
		l.sw	104(r3), r27
		l.sw	108(r3), r28
		l.sw	112(r3), r29
		l.sw	116(r3), r30
		l.sw	120(r3), r31

		l.lwz	r1, 0(r4)
		l.lwz	r2, 4(r4)
		l.lwz	r5, 16(r4)
		l.lwz	r6, 20(r4)
		l.lwz	r7, 24(r4)
		l.lwz	r8, 28(r4)
		l.lwz	r9, 32(r4)
		l.lwz	r10, 36(r4)
		l.lwz	r11, 40(r4)
		l.lwz	r12, 44(r4)
		l.lwz	r13, 48(r4)
		l.lwz	r14, 52(r4)
		l.lwz	r15, 56(r4)
		l.lwz	r16, 60(r4)
		l.lwz	r17, 64(r4)
		l.lwz	r18, 68(r4)
		l.lwz	r19, 72(r4)
		l.lwz	r20, 76(r4)
		l.lwz	r21, 80(r4)
		l.lwz	r22, 84(r4)
		l.lwz	r23, 88(r4)
		l.lwz	r24, 92(r4)
		l.lwz	r25, 96(r4)
		l.lwz	r26, 100(r4)
		l.lwz	r27, 104(r4)
		l.lwz	r28, 108(r4)
		l.lwz	r29, 112(r4)
		l.lwz	r30, 116(r4)
		l.lwz	r31, 120(r4)
		
		l.jr	r9
		l.nop