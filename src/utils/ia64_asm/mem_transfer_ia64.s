	.file	"mem_transfer.c"
	.pred.safe_across_calls p1-p5,p16-p63
	.common	transfer_8to16copy#,8,8
.text
	.align 16
	.global transfer_8to16copy_ia64#
	.proc transfer_8to16copy_ia64#
transfer_8to16copy_ia64:
	.prologue
	.save ar.lc, r2
	mov r2 = ar.lc
	.body
	addl r14 = 7, r0
	mov r21 = r0
	mov r20 = r0
	;;
	mov ar.lc = r14
	;;
.L101:
	addl r19 = 1, r0
	zxt4 r14 = r21
	dep.z r15 = r20, 1, 32
	;;
	add r16 = r21, r19
	add r14 = r33, r14
	add r17 = r20, r19
	;;
	ld1 r18 = [r14]
	add r15 = r15, r32
	zxt4 r16 = r16
	;;
	st2 [r15] = r18
	addl r19 = 2, r0
	add r16 = r33, r16
	dep.z r17 = r17, 1, 32
	;;
	ld1 r15 = [r16]
	add r14 = r21, r19
	add r18 = r20, r19
	add r17 = r17, r32
	;;
	zxt4 r14 = r14
	st2 [r17] = r15
	addl r19 = 3, r0
	;;
	add r14 = r33, r14
	add r15 = r21, r19
	dep.z r18 = r18, 1, 32
	;;
	ld1 r17 = [r14]
	add r16 = r20, r19
	add r18 = r18, r32
	zxt4 r15 = r15
	;;
	st2 [r18] = r17
	addl r19 = 4, r0
	add r15 = r33, r15
	dep.z r16 = r16, 1, 32
	;;
	ld1 r18 = [r15]
	add r14 = r21, r19
	add r17 = r20, r19
	add r16 = r16, r32
	;;
	zxt4 r14 = r14
	st2 [r16] = r18
	addl r19 = 5, r0
	;;
	add r14 = r33, r14
	add r15 = r21, r19
	add r16 = r20, r19
	dep.z r17 = r17, 1, 32
	;;
	ld1 r18 = [r14]
	addl r19 = 6, r0
	add r17 = r17, r32
	zxt4 r15 = r15
	;;
	st2 [r17] = r18
	add r14 = r21, r19
	add r15 = r33, r15
	dep.z r16 = r16, 1, 32
	add r17 = r20, r19
	;;
	ld1 r18 = [r15]
	add r16 = r16, r32
	zxt4 r14 = r14
	;;
	st2 [r16] = r18
	addl r19 = 7, r0
	add r14 = r33, r14
	;;
	ld1 r15 = [r14]
	add r16 = r21, r19
	dep.z r17 = r17, 1, 32
	add r14 = r20, r19
	;;
	add r17 = r17, r32
	zxt4 r16 = r16
	;;
	st2 [r17] = r15
	dep.z r14 = r14, 1, 32
	add r16 = r33, r16
	;;
	add r14 = r14, r32
	ld1 r15 = [r16]
	add r21 = r21, r34
	;;
	st2 [r14] = r15
	adds r20 = 8, r20
	br.cloop.sptk.few .L101
	;;
	mov ar.lc = r2
	br.ret.sptk.many b0
	.endp transfer_8to16copy_ia64#
	.common	transfer_16to8copy#,8,8
	.align 16
	.global transfer_16to8copy_ia64#
	.proc transfer_16to8copy_ia64#
transfer_16to8copy_ia64:
	.prologue
	.body
	mov r22 = r0
	addl r21 = 255, r0
	mov r20 = r0
	mov r19 = r0
.L25:
	mov r18 = r0
	;;
.L29:
	add r14 = r19, r18
	;;
	dep.z r14 = r14, 1, 32
	;;
	add r14 = r14, r33
	;;
	ld2 r15 = [r14]
	;;
	sxt2 r15 = r15
	;;
	mov r16 = r15
	;;
	cmp4.le p6, p7 = r0, r16
	;;
	(p7) mov r16 = r0
	(p7) br.cond.dpnt .L106
	;;
	cmp4.ge p6, p7 = r21, r16
	;;
	(p7) addl r16 = 255, r0
.L106:
	add r14 = r20, r18
	adds r17 = 1, r18
	;;
	zxt4 r14 = r14
	add r15 = r19, r17
	;;
	add r14 = r32, r14
	dep.z r15 = r15, 1, 32
	;;
	st1 [r14] = r16
	add r15 = r15, r33
	;;
	ld2 r14 = [r15]
	;;
	sxt2 r14 = r14
	;;
	mov r16 = r14
	;;
	cmp4.le p6, p7 = r0, r16
	;;
	(p7) mov r16 = r0
	(p7) br.cond.dpnt .L110
	;;
	cmp4.ge p6, p7 = r21, r16
	;;
	(p7) addl r16 = 255, r0
.L110:
	add r14 = r20, r17
	adds r17 = 2, r18
	;;
	zxt4 r14 = r14
	add r15 = r19, r17
	;;
	add r14 = r32, r14
	dep.z r15 = r15, 1, 32
	;;
	st1 [r14] = r16
	add r15 = r15, r33
	;;
	ld2 r14 = [r15]
	;;
	sxt2 r14 = r14
	;;
	mov r16 = r14
	;;
	cmp4.le p6, p7 = r0, r16
	;;
	(p7) mov r16 = r0
	(p7) br.cond.dpnt .L114
	;;
	cmp4.ge p6, p7 = r21, r16
	;;
	(p7) addl r16 = 255, r0
.L114:
	add r14 = r20, r17
	adds r17 = 3, r18
	;;
	zxt4 r14 = r14
	add r15 = r19, r17
	;;
	add r14 = r32, r14
	dep.z r15 = r15, 1, 32
	;;
	st1 [r14] = r16
	add r15 = r15, r33
	;;
	ld2 r14 = [r15]
	;;
	sxt2 r14 = r14
	;;
	mov r15 = r14
	;;
	cmp4.le p6, p7 = r0, r15
	;;
	(p7) mov r15 = r0
	(p7) br.cond.dpnt .L118
	;;
	cmp4.ge p6, p7 = r21, r15
	;;
	(p7) addl r15 = 255, r0
.L118:
	add r14 = r20, r17
	adds r18 = 4, r18
	;;
	zxt4 r14 = r14
	cmp4.geu p6, p7 = 7, r18
	;;
	add r14 = r32, r14
	;;
	st1 [r14] = r15
	(p6) br.cond.dptk .L29
	adds r22 = 1, r22
	add r20 = r20, r34
	adds r19 = 8, r19
	;;
	cmp4.geu p6, p7 = 7, r22
	(p6) br.cond.dptk .L25
	br.ret.sptk.many b0
	.endp transfer_16to8copy_ia64#
	.common	transfer_8to16sub#,8,8
	.align 16
	.global transfer_8to16sub_ia64#
	.proc transfer_8to16sub_ia64#
transfer_8to16sub_ia64:
	.prologue
	.body
	mov r25 = r0
	mov r24 = r0
	mov r23 = r0
.L39:
	mov r22 = r0
	;;
.L43:
	add r15 = r23, r22
	adds r20 = 1, r22
	add r16 = r24, r22
	;;
	zxt4 r15 = r15
	add r18 = r23, r20
	dep.z r16 = r16, 1, 32
	;;
	add r19 = r34, r15
	zxt4 r18 = r18
	add r16 = r16, r32
	add r15 = r33, r15
	;;
	ld1 r14 = [r19]
	add r21 = r34, r18
	ld1 r17 = [r15]
	adds r19 = 2, r22
	add r18 = r33, r18
	;;
	st1 [r15] = r14
	sub r17 = r17, r14
	add r20 = r24, r20
	;;
	st2 [r16] = r17
	dep.z r20 = r20, 1, 32
	ld1 r14 = [r21]
	ld1 r15 = [r18]
	add r16 = r23, r19
	;;
	st1 [r18] = r14
	sub r15 = r15, r14
	zxt4 r16 = r16
	add r20 = r20, r32
	;;
	add r18 = r34, r16
	adds r17 = 3, r22
	st2 [r20] = r15
	add r16 = r33, r16
	add r19 = r24, r19
	;;
	ld1 r14 = [r18]
	add r15 = r23, r17
	dep.z r19 = r19, 1, 32
	ld1 r18 = [r16]
	;;
	zxt4 r15 = r15
	add r19 = r19, r32
	st1 [r16] = r14
	sub r18 = r18, r14
	;;
	add r20 = r34, r15
	st2 [r19] = r18
	add r15 = r33, r15
	add r17 = r24, r17
	;;
	ld1 r14 = [r20]
	ld1 r16 = [r15]
	dep.z r17 = r17, 1, 32
	;;
	add r17 = r17, r32
	adds r22 = 4, r22
	st1 [r15] = r14
	sub r16 = r16, r14
	;;
	cmp4.geu p6, p7 = 7, r22
	st2 [r17] = r16
	(p6) br.cond.dptk .L43
	adds r25 = 1, r25
	adds r24 = 8, r24
	add r23 = r23, r35
	;;
	cmp4.geu p6, p7 = 7, r25
	(p6) br.cond.dptk .L39
	br.ret.sptk.many b0
	.endp transfer_8to16sub_ia64#
	.common	transfer_8to16sub2#,8,8
	.align 16
	.global transfer_8to16sub2_ia64#
	.proc transfer_8to16sub2_ia64#
transfer_8to16sub2_ia64:
	.prologue
	.save ar.lc, r2
	mov r2 = ar.lc
	.body
	mov r28 = r0
	addl r27 = 255, r0
	mov r26 = r0
	mov r25 = r0
.L50:
	addl r14 = 3, r0
	mov r21 = r0
	;;
	mov ar.lc = r14
	;;
.L138:
	add r14 = r26, r21
	add r17 = r25, r21
	adds r19 = 1, r21
	;;
	zxt4 r17 = r17
	dep.z r14 = r14, 1, 32
	add r18 = r25, r19
	;;
	add r15 = r34, r17
	add r23 = r14, r32
	add r20 = r35, r17
	;;
	ld1 r14 = [r15]
	ld1 r16 = [r20]
	add r17 = r33, r17
	;;
	add r14 = r14, r16
	ld1 r15 = [r17]
	zxt4 r18 = r18
	;;
	adds r14 = 1, r14
	add r24 = r35, r18
	add r22 = r34, r18
	;;
	shr.u r14 = r14, 1
	add r19 = r26, r19
	add r16 = r33, r18
	;;
	cmp4.ge p6, p7 = r27, r14
	dep.z r19 = r19, 1, 32
	adds r21 = 2, r21
	;;
	(p7) addl r14 = 255, r0
	add r19 = r19, r32
	;;
	sub r14 = r15, r14
	;;
	st2 [r23] = r14
	ld1 r14 = [r24]
	ld1 r15 = [r22]
	ld1 r16 = [r16]
	;;
	add r15 = r15, r14
	;;
	adds r15 = 1, r15
	;;
	shr.u r14 = r15, 1
	;;
	cmp4.ge p6, p7 = r27, r14
	;;
	(p7) addl r14 = 255, r0
	;;
	sub r14 = r16, r14
	;;
	st2 [r19] = r14
	br.cloop.sptk.few .L138
	adds r28 = 1, r28
	adds r26 = 8, r26
	add r25 = r25, r36
	;;
	cmp4.geu p6, p7 = 7, r28
	(p6) br.cond.dptk .L50
	mov ar.lc = r2
	br.ret.sptk.many b0
	.endp transfer_8to16sub2_ia64#
	.common	transfer_16to8add#,8,8
	.align 16
	.global transfer_16to8add_ia64#
	.proc transfer_16to8add_ia64#
transfer_16to8add_ia64:
	.prologue
	.save ar.lc, r2
	mov r2 = ar.lc
	.body
	mov r26 = r0
	addl r25 = 255, r0
	mov r24 = r0
	mov r21 = r0
.L62:
	addl r14 = 3, r0
	mov r20 = r0
	;;
	mov ar.lc = r14
	;;
.L149:
	adds r17 = 1, r20
	add r14 = r21, r20
	add r15 = r24, r20
	;;
	zxt4 r14 = r14
	add r18 = r21, r17
	dep.z r15 = r15, 1, 32
	;;
	add r23 = r32, r14
	zxt4 r18 = r18
	add r15 = r15, r33
	;;
	mov r16 = r23
	add r22 = r32, r18
	ld2 r14 = [r15]
	;;
	ld1 r18 = [r16]
	add r19 = r24, r17
	adds r20 = 2, r20
	;;
	add r14 = r14, r18
	dep.z r19 = r19, 1, 32
	mov r16 = r22
	;;
	sxt2 r14 = r14
	add r19 = r19, r33
	;;
	cmp4.le p6, p7 = r0, r14
	cmp4.ge p8, p9 = r25, r14
	;;
	(p7) mov r14 = r0
	(p7) br.cond.dpnt .L143
	;;
	(p9) addl r14 = 255, r0
	;;
.L143:
	st1 [r23] = r14
	ld1 r14 = [r22]
	ld2 r15 = [r19]
	;;
	add r15 = r15, r14
	;;
	sxt2 r15 = r15
	;;
	cmp4.le p6, p7 = r0, r15
	cmp4.ge p8, p9 = r25, r15
	;;
	(p7) mov r15 = r0
	(p7) br.cond.dpnt .L147
	;;
	(p9) addl r15 = 255, r0
	;;
.L147:
	st1 [r16] = r15
	br.cloop.sptk.few .L149
	adds r26 = 1, r26
	adds r24 = 8, r24
	add r21 = r21, r34
	;;
	cmp4.geu p6, p7 = 7, r26
	(p6) br.cond.dptk .L62
	mov ar.lc = r2
	br.ret.sptk.many b0
	.endp transfer_16to8add_ia64#
	.common	transfer8x8_copy#,8,8
	.align 16
	.global transfer8x8_copy_ia64#
	.proc transfer8x8_copy_ia64#
transfer8x8_copy_ia64:
	.prologue
	.save ar.lc, r2
	mov r2 = ar.lc
	.body
	addl r14 = 7, r0
	mov r21 = r0
	;;
	mov ar.lc = r14
	;;
.L168:
	zxt4 r14 = r21
	adds r15 = 1, r21
	adds r18 = 2, r21
	;;
	add r16 = r33, r14
	zxt4 r15 = r15
	zxt4 r18 = r18
	;;
	ld1 r17 = [r16]
	add r14 = r32, r14
	add r19 = r33, r15
	;;
	st1 [r14] = r17
	add r15 = r32, r15
	add r20 = r33, r18
	ld1 r16 = [r19]
	adds r14 = 3, r21
	add r18 = r32, r18
	;;
	st1 [r15] = r16
	zxt4 r14 = r14
	adds r17 = 4, r21
	ld1 r15 = [r20]
	;;
	add r19 = r33, r14
	zxt4 r17 = r17
	st1 [r18] = r15
	add r14 = r32, r14
	;;
	add r20 = r33, r17
	ld1 r15 = [r19]
	adds r16 = 5, r21
	add r17 = r32, r17
	;;
	st1 [r14] = r15
	zxt4 r16 = r16
	adds r18 = 6, r21
	ld1 r14 = [r20]
	;;
	add r19 = r33, r16
	zxt4 r18 = r18
	st1 [r17] = r14
	add r16 = r32, r16
	;;
	add r20 = r33, r18
	ld1 r14 = [r19]
	adds r15 = 7, r21
	add r18 = r32, r18
	;;
	st1 [r16] = r14
	zxt4 r15 = r15
	add r21 = r21, r34
	ld1 r16 = [r20]
	;;
	add r17 = r33, r15
	st1 [r18] = r16
	add r15 = r32, r15
	;;
	ld1 r14 = [r17]
	;;
	st1 [r15] = r14
	br.cloop.sptk.few .L168
	;;
	mov ar.lc = r2
	br.ret.sptk.many b0
	.endp transfer8x8_copy_ia64#
	.ident	"GCC: (GNU) 2.96 20000731 (Red Hat Linux 7.1 2.96-85)"
