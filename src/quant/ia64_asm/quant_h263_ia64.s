	.file	"quant_h263.c"
	.pred.safe_across_calls p1-p5,p16-p63
		.section	.rodata
	.align 4
	.type	 multipliers#,@object
	.size	 multipliers#,128
multipliers:
	data4	0
	data4	32769
	data4	16385
	data4	10923
	data4	8193
	data4	6554
	data4	5462
	data4	4682
	data4	4097
	data4	3641
	data4	3277
	data4	2979
	data4	2731
	data4	2521
	data4	2341
	data4	2185
	data4	2049
	data4	1928
	data4	1821
	data4	1725
	data4	1639
	data4	1561
	data4	1490
	data4	1425
	data4	1366
	data4	1311
	data4	1261
	data4	1214
	data4	1171
	data4	1130
	data4	1093
	data4	1058
	.global __divdi3#
.text
	.align 16
	.global quant_intra_ia64#
	.proc quant_intra_ia64#
quant_intra_ia64:
	.prologue //12, 37
	.save ar.pfs, r38
	alloc r38 = ar.pfs, 4, 3, 2, 0
	adds r16 = -8, r12
	.fframe 32
	adds r12 = -32, r12
	mov r17 = ar.lc
	addl r14 = @ltoff(multipliers#), gp
	ld2 r15 = [r33]
	;;
	.savesp ar.lc, 24
	st8 [r16] = r17, 8
	ld8 r14 = [r14]
	sxt2 r15 = r15
	;;
	.save.f 0x1
	stf.spill [r16] = f2
	.save rp, r37
	mov r37 = b0
	.body
	dep.z r36 = r34, 1, 15
	dep.z r16 = r34, 2, 32
	cmp4.ge p6, p7 = 0, r15
	;;
	add r16 = r16, r14
	;;
	ld4 r16 = [r16]
	;;
	setf.sig f2 = r16
	(p6) br.cond.dptk .L4
	extr r39 = r35, 1, 31
	sxt4 r40 = r35
	;;
	add r39 = r39, r15
	br .L38
	;;
.L4:
	extr r39 = r35, 1, 31
	sxt4 r40 = r35
	;;
	sub r39 = r15, r39
	;;
.L38:
	sxt4 r39 = r39
	br.call.sptk.many b0 = __divdi3#
	;;
	addl r16 = 2, r0
	st2 [r32] = r8
	addl r17 = 1, r0
	;;
	add r14 = r33, r16
	;;
	ld2 r15 = [r14]
	;;
	sxt2 r15 = r15
	;;
	mov r14 = r15
	;;
	cmp4.le p6, p7 = r0, r14
	(p6) br.cond.dptk .L21
	sub r14 = r0, r14
	;;
	sxt2 r14 = r14
	;;
	cmp4.le p6, p7 = r36, r14
	;;
	(p7) add r14 = r32, r16
	(p6) add r15 = r32, r16
	(p6) setf.sig f6 = r14
	;;
	(p7) st2 [r14] = r0
	(p6) xma.l f6 = f6, f2, f0
	;;
	(p6) getf.sig r14 = f6
	;;
	(p6) extr r14 = r14, 16, 16
	;;
	(p6) sub r14 = r0, r14
	br .L39
	;;
.L21:
	cmp4.le p6, p7 = r36, r14
	;;
	(p7) add r14 = r32, r16
	(p6) setf.sig f6 = r15
	;;
	(p7) st2 [r14] = r0
	(p6) xma.l f6 = f6, f2, f0
	(p6) add r15 = r32, r16
	;;
	(p6) getf.sig r14 = f6
	;;
	(p6) extr r14 = r14, 16, 16
.L39:
	//.pred.rel.mutex p6, p7
	;;
	(p6) st2 [r15] = r14
	adds r17 = 1, r17
	;;
	cmp4.geu p6, p7 = 63, r17
	(p7) br.cond.dptk .L16
	addl r14 = 30, r0
	;;
	mov ar.lc = r14
	;;
.L37:
	dep.z r16 = r17, 1, 32
	;;
	add r14 = r16, r33
	;;
	ld2 r15 = [r14]
	;;
	sxt2 r15 = r15
	;;
	mov r14 = r15
	;;
	cmp4.le p6, p7 = r0, r14
	(p6) br.cond.dptk .L27
	sub r14 = r0, r14
	;;
	sxt2 r14 = r14
	;;
	cmp4.le p6, p7 = r36, r14
	;;
	(p7) add r14 = r16, r32
	(p6) add r15 = r16, r32
	(p6) setf.sig f6 = r14
	;;
	(p7) st2 [r14] = r0
	(p6) xma.l f6 = f6, f2, f0
	;;
	(p6) getf.sig r14 = f6
	;;
	(p6) extr r14 = r14, 16, 16
	;;
	(p6) sub r14 = r0, r14
	br .L40
	;;
.L27:
	cmp4.le p6, p7 = r36, r14
	;;
	(p7) add r14 = r16, r32
	(p6) setf.sig f6 = r15
	;;
	(p7) st2 [r14] = r0
	(p6) xma.l f6 = f6, f2, f0
	(p6) add r15 = r16, r32
	;;
	(p6) getf.sig r14 = f6
	;;
	(p6) extr r14 = r14, 16, 16
.L40:
	//.pred.rel.mutex p6, p7
	;;
	(p6) st2 [r15] = r14
	adds r14 = 1, r17
	;;
	dep.z r16 = r14, 1, 32
	;;
	add r15 = r16, r33
	;;
	ld2 r14 = [r15]
	;;
	sxt2 r14 = r14
	;;
	mov r15 = r14
	;;
	cmp4.le p6, p7 = r0, r15
	(p6) br.cond.dptk .L33
	sub r14 = r0, r15
	;;
	sxt2 r14 = r14
	;;
	mov r15 = r14
	;;
	cmp4.le p6, p7 = r36, r15
	;;
	(p7) add r14 = r16, r32
	(p6) setf.sig f6 = r15
	;;
	(p7) st2 [r14] = r0
	(p6) xma.l f6 = f6, f2, f0
	(p6) add r15 = r16, r32
	;;
	(p6) getf.sig r14 = f6
	;;
	(p6) extr r14 = r14, 16, 16
	;;
	(p6) sub r14 = r0, r14
	br .L41
.L33:
	cmp4.le p6, p7 = r36, r15
	;;
	(p7) add r14 = r16, r32
	(p6) add r15 = r16, r32
	(p6) setf.sig f6 = r14
	;;
	(p7) st2 [r14] = r0
	(p6) xma.l f6 = f6, f2, f0
	;;
	(p6) getf.sig r14 = f6
	;;
	(p6) extr r14 = r14, 16, 16
.L41:
	//.pred.rel.mutex p6, p7
	;;
	(p6) st2 [r15] = r14
	adds r17 = 2, r17
	br.cloop.sptk.few .L37
.L16:
	adds r18 = 24, r12
	;;
	ld8 r19 = [r18], 8
	mov ar.pfs = r38
	mov b0 = r37
	;;
	mov ar.lc = r19
	ldf.fill f2 = [r18]
	.restore sp
	adds r12 = 32, r12
	br.ret.sptk.many b0
	.endp quant_intra_ia64#
	.align 16
	.global quant_inter_ia64#
	.proc quant_inter_ia64#
quant_inter_ia64:
	.prologue
	addl r14 = @ltoff(multipliers#), gp
	dep.z r15 = r34, 2, 32
	.save ar.lc, r2
	mov r2 = ar.lc
	;;
	.body
	ld8 r14 = [r14]
	extr.u r16 = r34, 1, 16
	dep.z r17 = r34, 1, 15
	;;
	add r15 = r15, r14
	mov r18 = r16
	mov r8 = r0
	;;
	ld4 r15 = [r15]
	addl r14 = 31, r0
	mov r19 = r0
	;;
	setf.sig f6 = r15
	mov ar.lc = r14
	;;
.L65:
	dep.z r16 = r19, 1, 32
	;;
	add r14 = r16, r33
	;;
	ld2 r15 = [r14]
	;;
	sxt2 r15 = r15
	;;
	mov r14 = r15
	;;
	cmp4.le p6, p7 = r0, r14
	(p6) br.cond.dptk .L55
	sub r14 = r0, r14
	;;
	sub r14 = r14, r18
	;;
	sxt2 r14 = r14
	;;
	cmp4.le p6, p7 = r17, r14
	;;
	(p7) add r14 = r16, r32
	(p6) setf.sig f7 = r14
	;;
	(p7) st2 [r14] = r0
	(p6) add r16 = r16, r32
	(p6) xma.l f7 = f7, f6, f0
	;;
	(p6) getf.sig r14 = f7
	;;
	(p6) extr r14 = r14, 16, 16
	;;
	(p6) sub r15 = r0, r14
	(p6) add r8 = r8, r14
	;;
	(p6) st2 [r16] = r15
	br .L53
.L55:
	sub r14 = r14, r18
	;;
	sxt2 r14 = r14
	;;
	cmp4.le p6, p7 = r17, r14
	;;
	(p7) add r14 = r16, r32
	(p6) add r15 = r16, r32
	(p6) setf.sig f7 = r14
	;;
	(p7) st2 [r14] = r0
	(p6) xma.l f7 = f7, f6, f0
	;;
	(p6) getf.sig r14 = f7
	;;
	(p6) extr r14 = r14, 16, 16
	;;
	(p6) st2 [r15] = r14
	(p6) add r8 = r8, r14
.L53:
	adds r14 = 1, r19
	;;
	dep.z r16 = r14, 1, 32
	;;
	add r15 = r16, r33
	;;
	ld2 r14 = [r15]
	;;
	sxt2 r14 = r14
	;;
	cmp4.le p6, p7 = r0, r14
	(p6) br.cond.dptk .L61
	sub r14 = r0, r14
	;;
	sub r14 = r14, r18
	;;
	sxt2 r14 = r14
	;;
	cmp4.le p6, p7 = r17, r14
	;;
	(p7) add r14 = r16, r32
	(p6) setf.sig f7 = r14
	;;
	(p7) st2 [r14] = r0
	(p6) add r16 = r16, r32
	(p6) xma.l f7 = f7, f6, f0
	;;
	(p6) getf.sig r14 = f7
	;;
	(p6) extr r14 = r14, 16, 16
	;;
	(p6) sub r15 = r0, r14
	(p6) add r8 = r8, r14
	;;
	(p6) st2 [r16] = r15
	br .L59
.L61:
	sub r14 = r14, r18
	;;
	sxt2 r14 = r14
	;;
	cmp4.le p6, p7 = r17, r14
	;;
	(p7) add r14 = r16, r32
	(p6) add r15 = r16, r32
	(p6) setf.sig f7 = r14
	;;
	(p7) st2 [r14] = r0
	(p6) xma.l f7 = f7, f6, f0
	;;
	(p6) getf.sig r14 = f7
	;;
	(p6) extr r14 = r14, 16, 16
	;;
	(p6) st2 [r15] = r14
	(p6) add r8 = r8, r14
.L59:
	adds r19 = 2, r19
	br.cloop.sptk.few .L65
	;;
	mov ar.lc = r2
	br.ret.sptk.many b0
	.endp quant_inter_ia64#
	.common	quant_intra#,8,8
	.common	dequant_intra#,8,8
	.align 16
	.global dequant_intra_ia64#
	.proc dequant_intra_ia64#
dequant_intra_ia64:
	.prologue
	ld2 r14 = [r33]
	andcm r15 = 1, r34
	setf.sig f8 = r35
	;;
	sxt2 r14 = r14
	sub r15 = r34, r15
	addl r16 = -2048, r0
	;;
	setf.sig f6 = r14
	setf.sig f7 = r15
	shladd r34 = r34, 1, r0
	;;
	xma.l f8 = f6, f8, f0
	.save ar.lc, r2
	mov r2 = ar.lc
	;;
	.body
	getf.sig r14 = f8
	setf.sig f6 = r34
	;;
	sxt2 r15 = r14
	st2 [r32] = r14
	;;
	cmp4.le p6, p7 = r16, r15
	;;
	(p7) st2 [r32] = r16
	(p7) br.cond.dptk .L68
	addl r14 = 2047, r0
	;;
	cmp4.ge p6, p7 = r14, r15
	;;
	(p7) st2 [r32] = r14
.L68:
	addl r14 = 20, r0
	addl r19 = 1, r0
	addl r21 = 2048, r0
	addl r20 = -2048, r0
	addl r18 = 2047, r0
	;;
	mov ar.lc = r14
	;;
.L110:
	dep.z r16 = r19, 1, 32
	;;
	add r14 = r16, r33
	;;
	ld2 r15 = [r14]
	;;
	sxt2 r15 = r15
	;;
	cmp4.ne p6, p7 = 0, r15
	;;
	(p7) add r14 = r16, r32
	;;
	(p7) st2 [r14] = r0
	(p7) br.cond.dpnt .L92
	cmp4.le p6, p7 = r0, r15
	(p6) br.cond.dptk .L95
	sub r14 = r0, r15
	add r17 = r16, r32
	;;
	setf.sig f8 = r14
	;;
	xma.l f8 = f6, f8, f7
	;;
	getf.sig r15 = f8
	;;
	cmp4.lt p6, p7 = r21, r15
	;;
	(p7) sub r14 = r0, r15
	;;
	(p7) st2 [r17] = r14
	(p6) st2 [r17] = r20
	br .L92
.L95:
	setf.sig f8 = r15
	add r14 = r16, r32
	;;
	xma.l f8 = f6, f8, f7
	;;
	getf.sig r15 = f8
	;;
	cmp4.le p6, p7 = r18, r15
	;;
	(p6) mov r15 = r18
	;;
	st2 [r14] = r15
.L92:
	adds r14 = 1, r19
	;;
	dep.z r17 = r14, 1, 32
	;;
	add r15 = r17, r33
	;;
	ld2 r14 = [r15]
	;;
	sxt2 r14 = r14
	;;
	mov r16 = r14
	;;
	cmp4.ne p6, p7 = 0, r16
	;;
	(p7) add r14 = r17, r32
	;;
	(p7) st2 [r14] = r0
	(p7) br.cond.dpnt .L98
	cmp4.le p6, p7 = r0, r16
	(p6) br.cond.dptk .L101
	sub r14 = r0, r16
	add r17 = r17, r32
	;;
	setf.sig f8 = r14
	;;
	xma.l f8 = f6, f8, f7
	;;
	getf.sig r16 = f8
	;;
	cmp4.lt p6, p7 = r21, r16
	;;
	(p7) sub r14 = r0, r16
	;;
	(p7) st2 [r17] = r14
	(p6) st2 [r17] = r20
	br .L98
.L101:
	setf.sig f8 = r16
	add r14 = r17, r32
	;;
	xma.l f8 = f6, f8, f7
	;;
	getf.sig r16 = f8
	;;
	cmp4.le p6, p7 = r18, r16
	;;
	(p6) mov r15 = r18
	(p7) mov r15 = r16
	;;
	st2 [r14] = r15
.L98:
	adds r14 = 2, r19
	;;
	dep.z r17 = r14, 1, 32
	;;
	add r15 = r17, r33
	;;
	ld2 r14 = [r15]
	;;
	sxt2 r14 = r14
	;;
	mov r16 = r14
	;;
	cmp4.ne p6, p7 = 0, r16
	;;
	(p7) add r14 = r17, r32
	;;
	(p7) st2 [r14] = r0
	(p7) br.cond.dpnt .L104
	cmp4.le p6, p7 = r0, r16
	(p6) br.cond.dptk .L107
	sub r14 = r0, r16
	add r17 = r17, r32
	;;
	setf.sig f8 = r14
	;;
	xma.l f8 = f6, f8, f7
	;;
	getf.sig r16 = f8
	;;
	cmp4.lt p6, p7 = r21, r16
	;;
	(p7) sub r14 = r0, r16
	;;
	(p7) st2 [r17] = r14
	(p6) st2 [r17] = r20
	br .L104
.L107:
	setf.sig f8 = r16
	add r14 = r17, r32
	;;
	xma.l f8 = f6, f8, f7
	;;
	getf.sig r16 = f8
	;;
	cmp4.le p6, p7 = r18, r16
	;;
	(p6) mov r15 = r18
	(p7) mov r15 = r16
	;;
	st2 [r14] = r15
.L104:
	adds r19 = 3, r19
	br.cloop.sptk.few .L110
	;;
	mov ar.lc = r2
	br.ret.sptk.many b0
	.endp dequant_intra_ia64#
	.common	quant_inter#,8,8
	.common	dequant_inter#,8,8
	.align 16
	.global dequant_inter_ia64#
	.proc dequant_inter_ia64#
dequant_inter_ia64:
	.prologue
	andcm r14 = 1, r34
	dep.z r15 = r34, 1, 15
	.save ar.lc, r2
	mov r2 = ar.lc
	;;
	.body
	sub r34 = r34, r14
	setf.sig f6 = r15
	mov r19 = r0
	addl r14 = 31, r0
	addl r18 = -2048, r0
	addl r17 = 2047, r0
	;;
	zxt2 r34 = r34
	mov ar.lc = r14
	;;
.L122:
	dep.z r16 = r19, 1, 32
	;;
	add r14 = r16, r33
	;;
	ld2 r15 = [r14]
	;;
	sxt2 r15 = r15
	;;
	mov r14 = r15
	;;
	cmp4.ne p6, p7 = 0, r14
	;;
	(p7) add r14 = r16, r32
	;;
	(p7) st2 [r14] = r0
	(p7) br.cond.dpnt .L112
	cmp4.le p6, p7 = r0, r14
	(p6) br.cond.dptk .L115
	setf.sig f7 = r14
	add r15 = r16, r32
	;;
	xma.l f7 = f7, f6, f0
	;;
	getf.sig r14 = f7
	;;
	sub r14 = r14, r34
	;;
	sxt2 r14 = r14
	;;
	cmp4.le p6, p7 = r18, r14
	;;
	(p7) mov r14 = r18
	br .L123
.L115:
	setf.sig f8 = r15
	setf.sig f7 = r34
	;;
	xma.l f8 = f8, f6, f7
	add r15 = r16, r32
	;;
	getf.sig r14 = f8
	;;
	sxt2 r14 = r14
	;;
	cmp4.le p6, p7 = r17, r14
	;;
	(p6) mov r14 = r17
	;;
.L123:
	st2 [r15] = r14
.L112:
	adds r14 = 1, r19
	;;
	dep.z r16 = r14, 1, 32
	;;
	add r15 = r16, r33
	;;
	ld2 r14 = [r15]
	;;
	sxt2 r14 = r14
	;;
	mov r15 = r14
	;;
	cmp4.ne p6, p7 = 0, r15
	;;
	(p7) add r14 = r16, r32
	;;
	(p7) st2 [r14] = r0
	(p7) br.cond.dpnt .L117
	cmp4.le p6, p7 = r0, r15
	(p6) br.cond.dptk .L120
	setf.sig f8 = r15
	;;
	xma.l f8 = f8, f6, f0
	add r15 = r16, r32
	;;
	getf.sig r14 = f8
	;;
	sub r14 = r14, r34
	;;
	sxt2 r14 = r14
	;;
	cmp4.le p6, p7 = r18, r14
	;;
	(p7) mov r14 = r18
	br .L124
	;;
.L120:
	setf.sig f7 = r14
	setf.sig f8 = r34
	add r15 = r16, r32
	;;
	xma.l f7 = f7, f6, f8
	;;
	getf.sig r14 = f7
	;;
	sxt2 r14 = r14
	;;
	cmp4.le p6, p7 = r17, r14
	;;
	(p6) mov r14 = r17
	;;
.L124:
	st2 [r15] = r14
.L117:
	adds r19 = 2, r19
	br.cloop.sptk.few .L122
	;;
	mov ar.lc = r2
	br.ret.sptk.many b0
	.endp dequant_inter_ia64#
	.ident	"GCC: (GNU) 2.96 20000731 (Red Hat Linux 7.1 2.96-85)"
