	.file	"interpolate8x8.c"
	.pred.safe_across_calls p1-p5,p16-p63
	.common	interpolate8x8_halfpel_h#,8,8
	.common	interpolate8x8_halfpel_v#,8,8
	.common	interpolate8x8_halfpel_hv#,8,8
.text
	.align 16
	.global interpolate8x8_halfpel_h_ia64#
	.proc interpolate8x8_halfpel_h_ia64#
interpolate8x8_halfpel_h_ia64:
	.prologue
	.body
	mov r26 = r0
	mov r25 = r0
.L15:
	mov r24 = r0
	;;
	adds r23 = 1, r25
.L19:
	add r18 = r25, r24
	;;
	zxt4 r15 = r23
	adds r21 = 1, r24
	zxt4 r18 = r18
	;;
	add r15 = r33, r15
	adds r17 = 1, r23
	;;
	ld1 r14 = [r15]
	add r16 = r33, r18
	add r21 = r25, r21
	;;
	ld1 r15 = [r16]
	zxt4 r21 = r21
	add r18 = r32, r18
	;;
	add r14 = r14, r15
	zxt4 r17 = r17
	add r16 = r33, r21
	;;
	sub r14 = r14, r35
	add r17 = r33, r17
	adds r19 = 2, r24
	;;
	adds r14 = 1, r14
	adds r20 = 2, r23
	add r19 = r25, r19
	;;
	extr r14 = r14, 1, 16
	zxt4 r19 = r19
	add r21 = r32, r21
	;;
	st1 [r18] = r14
	zxt4 r20 = r20
	add r22 = r33, r19
	ld1 r15 = [r16]
	ld1 r14 = [r17]
	;;
	add r20 = r33, r20
	add r14 = r14, r15
	adds r16 = 3, r24
	adds r17 = 3, r23
	;;
	sub r14 = r14, r35
	add r16 = r25, r16
	add r19 = r32, r19
	;;
	adds r14 = 1, r14
	zxt4 r16 = r16
	zxt4 r17 = r17
	;;
	extr r14 = r14, 1, 16
	add r18 = r33, r16
	add r17 = r33, r17
	;;
	st1 [r21] = r14
	add r16 = r32, r16
	adds r24 = 4, r24
	ld1 r15 = [r22]
	ld1 r14 = [r20]
	adds r23 = 4, r23
	;;
	add r14 = r14, r15
	cmp4.geu p6, p7 = 7, r24
	;;
	sub r14 = r14, r35
	;;
	adds r14 = 1, r14
	;;
	extr r14 = r14, 1, 16
	;;
	st1 [r19] = r14
	ld1 r15 = [r18]
	ld1 r14 = [r17]
	;;
	add r14 = r14, r15
	;;
	sub r14 = r14, r35
	;;
	adds r14 = 1, r14
	;;
	extr r14 = r14, 1, 16
	;;
	st1 [r16] = r14
	(p6) br.cond.dptk .L19
	adds r26 = 1, r26
	add r25 = r25, r34
	;;
	cmp4.geu p6, p7 = 7, r26
	(p6) br.cond.dptk .L15
	br.ret.sptk.many b0
	.endp interpolate8x8_halfpel_h_ia64#
	.align 16
	.global interpolate8x8_halfpel_v_ia64#
	.proc interpolate8x8_halfpel_v_ia64#
interpolate8x8_halfpel_v_ia64:
	.prologue
	.body
	mov r26 = r0
	mov r25 = r0
.L26:
	mov r24 = r0
	;;
	add r23 = r25, r34
.L30:
	add r18 = r25, r24
	;;
	zxt4 r15 = r23
	adds r21 = 1, r24
	zxt4 r18 = r18
	;;
	add r15 = r33, r15
	adds r17 = 1, r23
	;;
	ld1 r14 = [r15]
	add r16 = r33, r18
	add r21 = r25, r21
	;;
	ld1 r15 = [r16]
	zxt4 r21 = r21
	add r18 = r32, r18
	;;
	add r14 = r14, r15
	zxt4 r17 = r17
	add r16 = r33, r21
	;;
	sub r14 = r14, r35
	add r17 = r33, r17
	adds r19 = 2, r24
	;;
	adds r14 = 1, r14
	adds r20 = 2, r23
	add r19 = r25, r19
	;;
	extr r14 = r14, 1, 16
	zxt4 r19 = r19
	add r21 = r32, r21
	;;
	st1 [r18] = r14
	zxt4 r20 = r20
	add r22 = r33, r19
	ld1 r15 = [r16]
	ld1 r14 = [r17]
	;;
	add r20 = r33, r20
	add r14 = r14, r15
	adds r16 = 3, r24
	adds r17 = 3, r23
	;;
	sub r14 = r14, r35
	add r16 = r25, r16
	add r19 = r32, r19
	;;
	adds r14 = 1, r14
	zxt4 r16 = r16
	zxt4 r17 = r17
	;;
	extr r14 = r14, 1, 16
	add r18 = r33, r16
	add r17 = r33, r17
	;;
	st1 [r21] = r14
	add r16 = r32, r16
	adds r24 = 4, r24
	ld1 r15 = [r22]
	ld1 r14 = [r20]
	adds r23 = 4, r23
	;;
	add r14 = r14, r15
	cmp4.geu p6, p7 = 7, r24
	;;
	sub r14 = r14, r35
	;;
	adds r14 = 1, r14
	;;
	extr r14 = r14, 1, 16
	;;
	st1 [r19] = r14
	ld1 r15 = [r18]
	ld1 r14 = [r17]
	;;
	add r14 = r14, r15
	;;
	sub r14 = r14, r35
	;;
	adds r14 = 1, r14
	;;
	extr r14 = r14, 1, 16
	;;
	st1 [r16] = r14
	(p6) br.cond.dptk .L30
	adds r26 = 1, r26
	add r25 = r25, r34
	;;
	cmp4.geu p6, p7 = 7, r26
	(p6) br.cond.dptk .L26
	br.ret.sptk.many b0
	.endp interpolate8x8_halfpel_v_ia64#
	.align 16
	.global interpolate8x8_halfpel_hv_ia64#
	.proc interpolate8x8_halfpel_hv_ia64#
interpolate8x8_halfpel_hv_ia64:
	.prologue
	.save ar.lc, r2
	mov r2 = ar.lc
	.body
	mov r27 = r0
	mov r26 = r0
	;;
.L37:
	add r14 = r26, r34
	mov r25 = r0
	adds r24 = 1, r26
	;;
	mov r23 = r14
	adds r22 = 1, r14
	addl r14 = 3, r0
	;;
	mov ar.lc = r14
	;;
.L70:
	add r21 = r26, r25
	zxt4 r15 = r24
	zxt4 r16 = r23
	;;
	zxt4 r21 = r21
	add r15 = r33, r15
	add r16 = r33, r16
	;;
	add r19 = r33, r21
	ld1 r17 = [r15]
	zxt4 r14 = r22
	;;
	ld1 r20 = [r19]
	ld1 r18 = [r16]
	add r14 = r33, r14
	;;
	add r17 = r17, r20
	ld1 r15 = [r14]
	adds r19 = 1, r24
	;;
	add r18 = r18, r17
	adds r20 = 1, r25
	adds r14 = 1, r23
	;;
	add r15 = r15, r18
	add r20 = r26, r20
	add r21 = r32, r21
	;;
	sub r15 = r15, r35
	zxt4 r20 = r20
	zxt4 r19 = r19
	;;
	adds r15 = 2, r15
	add r17 = r33, r20
	adds r16 = 1, r22
	;;
	extr r15 = r15, 2, 16
	add r19 = r33, r19
	zxt4 r14 = r14
	;;
	st1 [r21] = r15
	add r14 = r33, r14
	zxt4 r16 = r16
	ld1 r18 = [r17]
	ld1 r15 = [r19]
	;;
	add r16 = r33, r16
	ld1 r17 = [r14]
	add r15 = r15, r18
	add r20 = r32, r20
	;;
	ld1 r14 = [r16]
	add r17 = r17, r15
	adds r22 = 2, r22
	;;
	add r14 = r14, r17
	adds r23 = 2, r23
	adds r24 = 2, r24
	;;
	sub r14 = r14, r35
	adds r25 = 2, r25
	;;
	adds r14 = 2, r14
	;;
	extr r14 = r14, 2, 16
	;;
	st1 [r20] = r14
	br.cloop.sptk.few .L70
	adds r27 = 1, r27
	add r26 = r26, r34
	;;
	cmp4.geu p6, p7 = 7, r27
	(p6) br.cond.dptk .L37
	mov ar.lc = r2
	br.ret.sptk.many b0
	.endp interpolate8x8_halfpel_hv_ia64#
	.ident	"GCC: (GNU) 2.96 20000731 (Red Hat Linux 7.1 2.96-85)"
