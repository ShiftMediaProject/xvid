	.file	"idct.c"
	.pred.safe_across_calls p1-p5,p16-p63
.sbss
	.align 8
	.type	 blk.0#,@object
	.size	 blk.0#,8
blk.0:
	.skip	8
	.align 8
	.type	 i.1#,@object
	.size	 i.1#,8
i.1:
	.skip	8
	.align 8
	.type	 X0.2#,@object
	.size	 X0.2#,8
X0.2:
	.skip	8
	.align 8
	.type	 X1.3#,@object
	.size	 X1.3#,8
X1.3:
	.skip	8
	.align 8
	.type	 X2.4#,@object
	.size	 X2.4#,8
X2.4:
	.skip	8
	.align 8
	.type	 X3.5#,@object
	.size	 X3.5#,8
X3.5:
	.skip	8
	.align 8
	.type	 X4.6#,@object
	.size	 X4.6#,8
X4.6:
	.skip	8
	.align 8
	.type	 X5.7#,@object
	.size	 X5.7#,8
X5.7:
	.skip	8
	.align 8
	.type	 X6.8#,@object
	.size	 X6.8#,8
X6.8:
	.skip	8
	.align 8
	.type	 X7.9#,@object
	.size	 X7.9#,8
X7.9:
	.skip	8
	.align 8
	.type	 X8.10#,@object
	.size	 X8.10#,8
X8.10:
	.skip	8
.text
	.align 16
	.global idct_ia64#
	.proc idct_ia64#
idct_ia64:
	.prologue
	alloc r16 = ar.pfs, 1, 4, 8, 0
	.body
	addl r14 = @gprel(i.1#), gp
	addl r33 = @gprel(blk.0#), gp
	addl r11 = @gprel(X1.3#), gp
	;;
	st8 [r14] = r0
	mov r41 = r14
	addl r10 = @gprel(X2.4#), gp
	addl r14 = 565, r0
	addl r9 = @gprel(X3.5#), gp
	addl r8 = @gprel(X4.6#), gp
	;;
	setf.sig f14 = r14
	addl r44 = @gprel(X5.7#), gp
	addl r43 = @gprel(X6.8#), gp
	addl r14 = 2276, r0
	addl r42 = @gprel(X7.9#), gp
	addl r3 = @gprel(X0.2#), gp
	;;
	setf.sig f15 = r14
	addl r2 = @gprel(X8.10#), gp
	addl r14 = 3406, r0
	;;
	setf.sig f13 = r14
	addl r14 = 2408, r0
	;;
	setf.sig f12 = r14
	addl r14 = 799, r0
	;;
	setf.sig f11 = r14
	addl r14 = 4017, r0
	;;
	setf.sig f9 = r14
	addl r14 = 1108, r0
	;;
	setf.sig f10 = r14
	addl r14 = 3784, r0
	;;
	setf.sig f8 = r14
	addl r14 = 181, r0
	;;
	setf.sig f7 = r14
.L6:
	ld8 r14 = [r41]
	;;
	shladd r27 = r14, 4, r32
	;;
	adds r40 = 12, r27
	adds r39 = 8, r27
	adds r31 = 2, r27
	;;
	ld2 r15 = [r40]
	ld2 r14 = [r39]
	adds r38 = 4, r27
	;;
	sxt2 r28 = r15
	sxt2 r14 = r14
	adds r37 = 14, r27
	ld2 r15 = [r31]
	;;
	shl r25 = r14, 11
	ld2 r16 = [r38]
	sxt2 r23 = r15
	;;
	st8 [r11] = r25
	sxt2 r20 = r16
	ld2 r15 = [r37]
	or r14 = r28, r25
	adds r30 = 10, r27
	;;
	sxt2 r22 = r15
	st8 [r10] = r28
	or r14 = r20, r14
	ld2 r15 = [r30]
	st8 [r9] = r20
	;;
	or r14 = r23, r14
	sxt2 r19 = r15
	adds r29 = 6, r27
	st8 [r8] = r23
	;;
	or r14 = r22, r14
	ld2 r15 = [r29]
	st8 [r44] = r22
	;;
	sxt2 r18 = r15
	or r14 = r19, r14
	st8 [r43] = r19
	;;
	or r14 = r18, r14
	st8 [r33] = r27
	st8 [r42] = r18
	;;
	cmp.ne p6, p7 = 0, r14
	(p6) br.cond.dptk .L7
	ld2 r14 = [r27]
	;;
	shladd r14 = r14, 3, r0
	;;
	st2 [r37] = r14
	st2 [r40] = r14
	st2 [r30] = r14
	st2 [r39] = r14
	st2 [r29] = r14
	st2 [r38] = r14
	st2 [r31] = r14
	st2 [r27] = r14
	br .L5
.L7:
	add r21 = r19, r18
	add r14 = r23, r22
	ld2 r17 = [r27]
	;;
	setf.sig f32 = r21
	setf.sig f6 = r14
	add r15 = r20, r28
	;;
	xma.l f32 = f32, f12, f0
	shladd r16 = r20, 1, r20
	sxt2 r17 = r17
	;;
	getf.sig r21 = f32
	xma.l f6 = f6, f14, f0
	shladd r16 = r16, 4, r20
	setf.sig f32 = r22
	;;
	getf.sig r14 = f6
	dep.z r17 = r17, 11, 21
	xma.l f32 = f32, f13, f0
	;;
	adds r17 = 128, r17
	shl r16 = r16, 5
	getf.sig r22 = f32
	;;
	sxt4 r17 = r17
	setf.sig f32 = r19
	sub r22 = r14, r22
	;;
	add r24 = r17, r25
	xma.l f32 = f32, f11, f0
	sub r17 = r17, r25
	;;
	getf.sig r19 = f32
	setf.sig f32 = r18
	;;
	sub r19 = r21, r19
	xma.l f32 = f32, f9, f0
	;;
	getf.sig r14 = f32
	setf.sig f32 = r23
	;;
	sub r21 = r21, r14
	xma.l f32 = f32, f15, f6
	;;
	sub r26 = r22, r21
	getf.sig r23 = f32
	setf.sig f6 = r15
	add r22 = r22, r21
	;;
	sub r18 = r23, r19
	xma.l f6 = f6, f10, f0
	setf.sig f32 = r28
	;;
	getf.sig r15 = f6
	add r20 = r18, r26
	xma.l f32 = f32, f8, f0
	;;
	setf.sig f6 = r20
	add r16 = r15, r16
	getf.sig r14 = f32
	sub r18 = r18, r26
	;;
	xma.l f6 = f6, f7, f0
	add r23 = r23, r19
	sub r15 = r15, r14
	setf.sig f32 = r18
	;;
	getf.sig r20 = f6
	add r25 = r24, r16
	add r19 = r17, r15
	;;
	adds r20 = 128, r20
	xma.l f32 = f32, f7, f0
	sub r17 = r17, r15
	;;
	shr r20 = r20, 8
	getf.sig r18 = f32
	add r14 = r25, r23
	;;
	add r15 = r19, r20
	shr r14 = r14, 8
	adds r18 = 128, r18
	st8 [r11] = r23
	;;
	st2 [r27] = r14
	shr r18 = r18, 8
	shr r15 = r15, 8
	;;
	st2 [r31] = r15
	st8 [r43] = r22
	sub r24 = r24, r16
	add r14 = r17, r18
	st8 [r44] = r26
	;;
	add r15 = r24, r22
	shr r14 = r14, 8
	st8 [r42] = r25
	;;
	shr r15 = r15, 8
	st2 [r38] = r14
	sub r16 = r24, r22
	st8 [r2] = r24
	;;
	st2 [r29] = r15
	shr r16 = r16, 8
	sub r14 = r17, r18
	;;
	st2 [r39] = r16
	st8 [r9] = r19
	sub r15 = r19, r20
	shr r14 = r14, 8
	st8 [r3] = r17
	sub r16 = r25, r23
	;;
	st2 [r30] = r14
	shr r15 = r15, 8
	shr r16 = r16, 8
	;;
	st2 [r40] = r15
	st8 [r10] = r20
	st2 [r37] = r16
	st8 [r8] = r18
.L5:
	ld8 r14 = [r41]
	;;
	adds r14 = 1, r14
	;;
	st8 [r41] = r14
	cmp.ge p6, p7 = 7, r14
	(p6) br.cond.dptk .L6
	addl r14 = @gprel(i.1#), gp
	addl r36 = @gprel(X8.10#), gp
	addl r35 = @gprel(blk.0#), gp
	;;
	st8 [r14] = r0
	mov r42 = r14
	addl r33 = @gprel(X1.3#), gp
	addl r14 = 565, r0
	addl r3 = @gprel(X2.4#), gp
	addl r11 = @gprel(X3.5#), gp
	;;
	setf.sig f14 = r14
	addl r10 = @gprel(X4.6#), gp
	addl r9 = @gprel(X5.7#), gp
	addl r14 = 2276, r0
	addl r8 = @gprel(X6.8#), gp
	addl r44 = @gprel(X7.9#), gp
	;;
	setf.sig f12 = r14
	addl r43 = @gprel(iclp_ia64#), gp
	addl r34 = @gprel(X0.2#), gp
	addl r14 = 3406, r0
	;;
	setf.sig f13 = r14
	addl r14 = 2408, r0
	;;
	setf.sig f10 = r14
	addl r14 = 799, r0
	;;
	setf.sig f11 = r14
	addl r14 = 4017, r0
	;;
	setf.sig f8 = r14
	addl r14 = 1108, r0
	;;
	setf.sig f9 = r14
	addl r14 = 3784, r0
	;;
	setf.sig f7 = r14
	addl r14 = 181, r0
	;;
	setf.sig f6 = r14
.L12:
	ld8 r14 = [r42]
	;;
	shladd r29 = r14, 1, r32
	;;
	adds r41 = 96, r29
	adds r39 = 64, r29
	adds r31 = 16, r29
	;;
	ld2 r15 = [r41]
	ld2 r14 = [r39]
	adds r37 = 32, r29
	;;
	sxt2 r25 = r15
	sxt2 r14 = r14
	adds r40 = 112, r29
	ld2 r15 = [r31]
	;;
	shl r26 = r14, 8
	ld2 r16 = [r37]
	sxt2 r23 = r15
	;;
	st8 [r33] = r26
	sxt2 r16 = r16
	ld2 r15 = [r40]
	or r14 = r25, r26
	adds r38 = 80, r29
	;;
	sxt2 r24 = r15
	st8 [r3] = r25
	or r14 = r16, r14
	ld2 r15 = [r38]
	st8 [r11] = r16
	;;
	or r14 = r23, r14
	sxt2 r22 = r15
	adds r30 = 48, r29
	st8 [r10] = r23
	;;
	or r14 = r24, r14
	ld2 r15 = [r30]
	st8 [r9] = r24
	;;
	sxt2 r20 = r15
	or r14 = r22, r14
	st8 [r8] = r22
	;;
	or r14 = r20, r14
	st8 [r35] = r29
	st8 [r44] = r20
	;;
	cmp.ne p6, p7 = 0, r14
	(p6) br.cond.dptk .L13
	ld2 r14 = [r29]
	ld8 r16 = [r43]
	;;
	sxt2 r14 = r14
	;;
	adds r14 = 32, r14
	;;
	extr r14 = r14, 6, 26
	;;
	shladd r14 = r14, 1, r16
	;;
	ld2 r15 = [r14]
	;;
	st2 [r40] = r15
	st2 [r41] = r15
	st2 [r38] = r15
	st2 [r39] = r15
	st2 [r30] = r15
	st2 [r37] = r15
	st2 [r31] = r15
	st2 [r29] = r15
	br .L11
.L13:
	add r19 = r23, r24
	add r21 = r22, r20
	add r17 = r16, r25
	;;
	setf.sig f15 = r19
	setf.sig f32 = r21
	ld2 r2 = [r29]
	;;
	xma.l f15 = f15, f14, f0
	shladd r18 = r16, 1, r16
	sxt2 r2 = r2
	;;
	getf.sig r19 = f15
	xma.l f32 = f32, f10, f0
	shladd r18 = r18, 4, r16
	setf.sig f15 = r24
	;;
	getf.sig r21 = f32
	adds r19 = 4, r19
	xma.l f15 = f15, f13, f0
	setf.sig f32 = r22
	;;
	adds r21 = 4, r21
	getf.sig r24 = f15
	xma.l f32 = f32, f11, f0
	dep.z r2 = r2, 8, 24
	setf.sig f15 = r20
	;;
	getf.sig r15 = f32
	sub r24 = r19, r24
	xma.l f15 = f15, f8, f0
	setf.sig f32 = r23
	;;
	sub r15 = r21, r15
	getf.sig r14 = f15
	;;
	shr r15 = r15, 3
	shr r24 = r24, 3
	setf.sig f15 = r19
	sub r21 = r21, r14
	addl r2 = 8192, r2
	;;
	xma.l f32 = f32, f12, f15
	shr r21 = r21, 3
	shl r18 = r18, 5
	;;
	getf.sig r19 = f32
	sub r28 = r24, r21
	setf.sig f15 = r25
	setf.sig f32 = r17
	;;
	shr r19 = r19, 3
	sxt4 r2 = r2
	xma.l f32 = f32, f9, f0
	;;
	sub r22 = r19, r15
	add r25 = r2, r26
	getf.sig r17 = f32
	;;
	add r23 = r22, r28
	xma.l f15 = f15, f7, f0
	adds r17 = 4, r17
	;;
	setf.sig f32 = r23
	getf.sig r14 = f15
	add r18 = r17, r18
	sub r22 = r22, r28
	;;
	xma.l f32 = f32, f6, f0
	shr r18 = r18, 3
	add r19 = r19, r15
	sub r17 = r17, r14
	;;
	add r27 = r25, r18
	setf.sig f15 = r22
	getf.sig r23 = f32
	shr r17 = r17, 3
	sub r2 = r2, r26
	;;
	adds r23 = 128, r23
	add r15 = r27, r19
	xma.l f15 = f15, f6, f0
	ld8 r20 = [r43]
	add r26 = r2, r17
	;;
	shr r23 = r23, 8
	getf.sig r22 = f15
	shr r15 = r15, 14
	;;
	adds r22 = 128, r22
	add r14 = r26, r23
	shladd r15 = r15, 1, r20
	sub r2 = r2, r17
	;;
	shr r22 = r22, 8
	ld2 r16 = [r15]
	shr r14 = r14, 14
	sub r25 = r25, r18
	;;
	st2 [r29] = r16
	shladd r14 = r14, 1, r20
	add r15 = r2, r22
	;;
	ld2 r16 = [r14]
	add r24 = r24, r21
	shr r15 = r15, 14
	;;
	st2 [r31] = r16
	shladd r15 = r15, 1, r20
	add r14 = r25, r24
	;;
	ld2 r16 = [r15]
	shr r14 = r14, 14
	st8 [r33] = r19
	;;
	st2 [r37] = r16
	shladd r14 = r14, 1, r20
	sub r15 = r25, r24
	;;
	ld2 r17 = [r14]
	shr r15 = r15, 14
	sub r16 = r2, r22
	;;
	st2 [r30] = r17
	shladd r15 = r15, 1, r20
	st8 [r8] = r24
	;;
	ld2 r17 = [r15]
	shr r16 = r16, 14
	st8 [r9] = r28
	;;
	st2 [r39] = r17
	sub r14 = r26, r23
	shladd r16 = r16, 1, r20
	st8 [r44] = r27
	;;
	ld2 r15 = [r16]
	shr r14 = r14, 14
	;;
	st2 [r38] = r15
	st8 [r36] = r25
	shladd r14 = r14, 1, r20
	sub r18 = r27, r19
	;;
	ld2 r15 = [r14]
	st8 [r11] = r26
	shr r18 = r18, 14
	;;
	st2 [r41] = r15
	st8 [r34] = r2
	shladd r18 = r18, 1, r20
	st8 [r3] = r23
	;;
	ld2 r14 = [r18]
	st8 [r10] = r22
	;;
	st2 [r40] = r14
.L11:
	ld8 r14 = [r42]
	;;
	adds r14 = 1, r14
	;;
	st8 [r42] = r14
	cmp.ge p6, p7 = 7, r14
	(p6) br.cond.dptk .L12
	br.ret.sptk.many b0
	.endp idct_ia64#
	.align 16
	.global idct_ia64_init#
	.proc idct_ia64_init#
idct_ia64_init:
	.prologue
	addl r14 = @ltoff(iclip_ia64#), gp
	.save ar.lc, r2
	mov r2 = ar.lc
	.body
	addl r15 = @gprel(iclp_ia64#), gp
	;;
	ld8 r14 = [r14]
	addl r16 = 255, r0
	addl r17 = -512, r0
	;;
	adds r14 = 1024, r14
	addl r19 = -256, r0
	addl r18 = 255, r0
	mov ar.lc = r16
	;;
	st8 [r15] = r14
	mov r20 = r14
.L42:
	sxt4 r14 = r17
	cmp4.gt p6, p7 = r19, r17
	;;
	shladd r15 = r14, 1, r20
	;;
	(p6) st2 [r15] = r19
	(p6) br.cond.dptk .L26
	cmp4.le p6, p7 = r18, r17
	;;
	(p6) mov r14 = r18
	(p7) mov r14 = r17
	;;
	st2 [r15] = r14
.L26:
	adds r15 = 1, r17
	;;
	sxt4 r14 = r15
	cmp4.gt p6, p7 = r19, r15
	;;
	shladd r16 = r14, 1, r20
	;;
	(p6) st2 [r16] = r19
	(p6) br.cond.dptk .L30
	cmp4.le p6, p7 = r18, r15
	;;
	(p6) mov r14 = r18
	(p7) mov r14 = r15
	;;
	st2 [r16] = r14
.L30:
	adds r15 = 2, r17
	;;
	sxt4 r14 = r15
	cmp4.gt p6, p7 = r19, r15
	;;
	shladd r16 = r14, 1, r20
	;;
	(p6) st2 [r16] = r19
	(p6) br.cond.dptk .L34
	cmp4.le p6, p7 = r18, r15
	;;
	(p6) mov r14 = r18
	(p7) mov r14 = r15
	;;
	st2 [r16] = r14
.L34:
	adds r15 = 3, r17
	;;
	sxt4 r14 = r15
	cmp4.gt p6, p7 = r19, r15
	;;
	shladd r16 = r14, 1, r20
	;;
	(p6) st2 [r16] = r19
	(p6) br.cond.dptk .L38
	cmp4.le p6, p7 = r18, r15
	;;
	(p6) mov r14 = r18
	(p7) mov r14 = r15
	;;
	st2 [r16] = r14
.L38:
	adds r17 = 4, r17
	br.cloop.sptk.few .L42
	;;
	mov ar.lc = r2
	br.ret.sptk.many b0
	.endp idct_ia64_init#
	.common	idct#,8,8
.bss
	.align 2
	.type	 iclip_ia64#,@object
	.size	 iclip_ia64#,2048
iclip_ia64:
	.skip	2048
.sbss
	.align 8
	.type	 iclp_ia64#,@object
	.size	 iclp_ia64#,8
iclp_ia64:
	.skip	8
	.ident	"GCC: (GNU) 2.96 20000731 (Red Hat Linux 7.1 2.96-85)"
