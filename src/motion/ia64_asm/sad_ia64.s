.text
	.align 16
	.global sad16_ia64#
	.proc sad16_ia64#
sad16_ia64:

_LL=3
_SL=1
_OL=1
_PL=1
_AL=1
	
	alloc r9=ar.pfs,4,44,0,48

	mov r8 = r0

	mov r20 = ar.lc
	mov r21 = pr

	dep.z r22		= r32, 3, 3 // erste 3 Bit mit 8 multiplizieren
	dep.z r23		= r33, 3, 3 // in r22 und r23 -> Schiebeflags

	and r14		= -8, r32 // Parameter in untere Register kopieren
	and r15		= -8, r33 // Ref Cur mit 11111...1000 and-en
	mov r16		= r34
	mov r17		= r35
	;;
	add r18		= 8, r14  // Adressenvorausberechnen
	add r19		= 8, r15
	
	sub r24		= 64, r22 // Schiftanzahl ausrechnen
	sub r25		= 64, r23
	
	add r26		= 16, r14 // Adressenvorausberechnen
	add r27		= 16, r15

	// Loop-counter initialisieren		
	mov ar.lc = 15			// Loop 16 mal durchlaufen
	mov ar.ec = _LL + _SL + _OL + _PL + _AL + _AL			// Die Loop am Schluss noch neun mal durchlaufen

	// Rotating Predicate Register zuruecksetzen und P16 auf 1
	mov pr.rot = 1 << 16	
	;;
	
	// Array-Konstrukte initialisieren
	.rotr _ald1[_LL+1], _ald2[_LL+1], _ald3[_LL+1], _ald4[_LL+1], _ald5[_LL+1], _ald6[_LL+1], _shru1[_SL+1], _shl1[_SL+1], _shru2[_SL], _shl2[_SL], _shru3[_SL], _shl3[_SL], _shru4[_SL], _shl4[_SL+1], _or1[_OL], _or2[_OL], _or3[_OL], _or4[_OL+1], _psadr1[_PL+1], _psadr2[_PL+1], _addr1[_AL+1]
	.rotp _aldp[_LL], _shp[_SL], _orp[_OL], _psadrp[_PL], _addrp1[_AL], _addrp2[_AL]
	
.L_loop_16:
	{.mmi
		(_aldp[0]) ld8 _ald1[0] = [r14], r16	// Cur Erste 8 Byte
		(_aldp[0]) ld8 _ald2[0] = [r18], r16    // Cur Zweite 8 Byte
		(_psadrp[0]) psad1 _psadr1[0] = _or2[0], _or4[0] // Psadden
	}
	{.mmi
		(_aldp[0]) ld8 _ald3[0] = [r26], r16    // Cur Dritte 8 Byte
		(_aldp[0]) ld8 _ald4[0] = [r15], r16	// Ref Erste 8 Byte
		(_psadrp[0]) psad1 _psadr2[0] = _or3[0], _or4[_OL]  // _or2 +1 
	}
	{.mmi
		(_aldp[0]) ld8 _ald5[0] = [r19], r16    // Ref Zweite 8 Byte
		(_aldp[0]) ld8 _ald6[0] = [r27], r16    // Ref Dritte 8 Byte
		(_shp[0]) shr.u _shru1[0] = _ald1[_LL], r22
	}
	{.mii
		(_orp[0]) or _or1[0]     = _shl2[0], _shru3[0] // _shru2 + 1 und _shl2 + 1
		(_shp[0]) shl _shl1[0]   = _ald2[_LL], r24
		(_shp[0]) shr.u _shru2[0] = _ald2[_LL], r22
	}
	{.mii
		(_orp[0]) or _or2[0]  = _shl3[0], _shru4[0]  // _shru3 + 1 und _shl3 + 1
		(_shp[0]) shl _shl2[0] = _ald3[_LL], r24
		(_shp[0]) shr.u _shru3[0] = _ald4[_LL], r23
	}
	{.mii
		(_orp[0]) or _or3[0]  = _shl4[0], _shl4[_SL] //_shru4 + 1 und _shl4 + 1
		(_shp[0]) shl _shl3[0] = _ald5[_LL], r25
		(_shp[0]) shr.u _shru4[0] = _ald5[_LL], r23
	}
	{.mmi
		(_orp[0]) or _or4[0]  = _shru1[_SL], _shl1[_SL]
		(_shp[0]) shl _shl4[0]= _ald6[_LL], r25
	}
	{.mmb
		(_addrp1[0]) add _addr1[0] = _psadr1[_PL], _psadr2[_PL] // Aufsummieren
		(_addrp2[0]) add r8 = r8, _addr1[_AL]
		br.ctop.sptk.few .L_loop_16
		;;
	}
	// Register zurueckschreiben
	mov ar.lc = r20
	mov pr = r21,-1
 	br.ret.sptk.many rp
	.endp sad16_ia64#

		
  	.align 16
	.global sad8_ia64#
	.proc sad8_ia64#

sad8_ia64:

LL=3
SL=1
OL=1
PL=1
AL=1

	alloc r9=ar.pfs,3,29,0,32
	mov r20 = ar.lc
	mov r21 = pr
	
	dep.z r22		= r32, 3, 3 // erste 3 Bit mit 8 multiplizieren
	dep.z r23		= r33, 3, 3 // in r22 und r23 -> Schiebeflags
	
	mov r8 = r0		     //   .   .   .   .
	and r14		= -8, r32 // 0xFFFFFFFFFFFFFFF8, r32
	and r15		= -8, r33 // 0xFFFFFFFFFFFFFFF8, r33
	mov r16		= r34
//	mov r17		= r35
	;; 

	add r18		= 8, r14
	add r19		= 8, r15
	
	sub r24		= 64, r22
	sub r25		= 64, r23
	
	// Loop-counter initialisieren		
	mov ar.lc = 7			// Loop 7 mal durchlaufen
	mov ar.ec = LL + SL + OL + PL + AL			// Die Loop am Schluss noch zehn mal durchlaufen

	// Rotating Predicate Register zuruecksetzen und P16 auf 1
	mov pr.rot = 1 << 16	
	;;
	.rotr ald1[LL+1], ald2[LL+1], ald3[LL+1], ald4[LL+1], shru1[SL+1], shl1[SL+1], shru2[SL+1], shl2[SL+1], or1[OL+1], or2[OL+1], psadr[PL+1], addr[AL+1]
	.rotp aldp[LL], shp[SL], orp[OL], psadrp[PL], addrp[AL]
.L_loop_8:
	{.mmi
		(aldp[0]) ld8 ald1[0] = [r14], r16	// Cur laden
		(aldp[0]) ld8 ald2[0] = [r18], r16
		(shp[0]) shr.u shru1[0] = ald1[LL], r22	// mergen
	}
	{.mii
		(orp[0]) or or1[0] = shru1[SL], shl1[SL]
		(shp[0]) shl shl1[0] = ald2[LL], r24
		(shp[0]) shr.u shru2[0] = ald3[LL], r23	// mergen	
	}
	{.mmi
		(aldp[0]) ld8 ald3[0] = [r15], r16	// Ref laden
		(aldp[0]) ld8 ald4[0] = [r19], r16
		(shp[0]) shl shl2[0]  = ald4[LL], r25
	}
	{.mmi
		(orp[0]) or or2[0] = shru2[SL], shl2[SL]
		(addrp[0]) add r8 = r8, psadr[PL]
		(psadrp[0]) psad1 psadr[0] = or1[OL], or2[OL]
	}
	{.mbb 
		br.ctop.sptk.few .L_loop_8
		;; 
	} 

	mov ar.lc = r20
	mov pr = r21,-1
	br.ret.sptk.many b0
	.endp sad8_ia64#
	
		
	.common	sad16bi#,8,8
	.align 16
	.global sad16bi_ia64#
	.proc sad16bi_ia64#
sad16bi_ia64:
	.prologue
	.save ar.lc, r2
	mov r2 = ar.lc
	.body
	zxt4 r35 = r35
	mov r8 = r0
	mov r23 = r0
	addl r22 = 255, r0
.L21:
	addl r14 = 7, r0
	mov r19 = r32
	mov r21 = r34
	mov r20 = r33
	;;
	mov ar.lc = r14
	;;
.L105:
	mov r17 = r20
	mov r18 = r21
	;;
	ld1 r14 = [r17], 1
	ld1 r15 = [r18], 1
	;;
	add r14 = r14, r15
	;;
	adds r14 = 1, r14
	;;
	shr.u r16 = r14, 1
	;;
	cmp4.le p6, p7 = r0, r16
	;;
	(p7) mov r16 = r0
	(p7) br.cond.dpnt .L96
	;;
	cmp4.ge p6, p7 = r22, r16
	;;
	(p7) addl r16 = 255, r0
.L96:
	ld1 r14 = [r19]
	adds r20 = 2, r20
	adds r21 = 2, r21
	;;
	sub r15 = r14, r16
	;;
	cmp4.ge p6, p7 = 0, r15
	;;
	(p6) sub r14 = r16, r14
	(p7) add r8 = r8, r15
	;;
	(p6) add r8 = r8, r14
	ld1 r15 = [r18]
	ld1 r14 = [r17]
	;;
	add r14 = r14, r15
	adds r17 = 1, r19
	;;
	adds r14 = 1, r14
	;;
	shr.u r16 = r14, 1
	;;
	cmp4.le p6, p7 = r0, r16
	;;
	(p7) mov r16 = r0
	(p7) br.cond.dpnt .L102
	;;
	cmp4.ge p6, p7 = r22, r16
	;;
	(p7) addl r16 = 255, r0
.L102:
	ld1 r14 = [r17]
	adds r19 = 2, r19
	;;
	sub r15 = r14, r16
	;;
	cmp4.ge p6, p7 = 0, r15
	;;
	(p7) add r8 = r8, r15
	(p6) sub r14 = r16, r14
	;;
	(p6) add r8 = r8, r14
	br.cloop.sptk.few .L105
	adds r23 = 1, r23
	add r32 = r32, r35
	add r33 = r33, r35
	add r34 = r34, r35
	;;
	cmp4.geu p6, p7 = 15, r23
	(p6) br.cond.dptk .L21
	mov ar.lc = r2
	br.ret.sptk.many b0
	.endp sad16bi_ia64#


	.common	dev16#,8,8
	.align 16
	.global dev16_ia64#
	.proc dev16_ia64#
dev16_ia64:
	.prologue
	zxt4 r33 = r33
	.save ar.lc, r2
	mov r2 = ar.lc
	.body
	mov r21 = r0
	mov r8 = r0
	mov r23 = r32
	mov r24 = r0
	;;
	mov r25 = r33
.L50:
	mov r22 = r0
	mov r20 = r23
	;;
.L54:
	mov r16 = r20
	adds r14 = 2, r20
	adds r15 = 3, r20
	;;
	ld1 r17 = [r16], 1
	ld1 r18 = [r14]
	ld1 r19 = [r15]
	;;
	ld1 r14 = [r16]
	add r21 = r17, r21
	adds r15 = 4, r20
	;;
	add r21 = r14, r21
	ld1 r16 = [r15]
	adds r22 = 8, r22
	;;
	add r21 = r18, r21
	adds r14 = 5, r20
	adds r15 = 6, r20
	;;
	add r21 = r19, r21
	ld1 r17 = [r14]
	ld1 r18 = [r15]
	;;
	add r21 = r16, r21
	adds r14 = 7, r20
	cmp4.geu p6, p7 = 15, r22
	;;
	add r21 = r17, r21
	ld1 r15 = [r14]
	adds r20 = 8, r20
	;;
	add r21 = r18, r21
	;;
	add r21 = r15, r21
	(p6) br.cond.dptk .L54
	adds r24 = 1, r24
	add r23 = r23, r25
	;;
	cmp4.geu p6, p7 = 15, r24
	(p6) br.cond.dptk .L50
	extr.u r14 = r21, 8, 24
	mov r23 = r32
	mov r24 = r0
	;;
	mov r21 = r14
.L60:
	addl r14 = 3, r0
	mov r17 = r23
	;;
	mov ar.lc = r14
	;;
.L144:
	mov r16 = r17
	;;
	ld1 r14 = [r16], 1
	;;
	sub r15 = r14, r21
	;;
	cmp4.ge p6, p7 = 0, r15
	;;
	(p7) add r8 = r8, r15
	(p6) sub r14 = r21, r14
	;;
	(p6) add r8 = r8, r14
	ld1 r14 = [r16]
	;;
	sub r15 = r14, r21
	adds r16 = 2, r17
	;;
	cmp4.ge p6, p7 = 0, r15
	;;
	(p7) add r8 = r8, r15
	(p6) sub r14 = r21, r14
	;;
	(p6) add r8 = r8, r14
	ld1 r14 = [r16]
	;;
	sub r15 = r14, r21
	adds r16 = 3, r17
	;;
	cmp4.ge p6, p7 = 0, r15
	adds r17 = 4, r17
	;;
	(p7) add r8 = r8, r15
	(p6) sub r14 = r21, r14
	;;
	(p6) add r8 = r8, r14
	ld1 r14 = [r16]
	;;
	sub r15 = r14, r21
	;;
	cmp4.ge p6, p7 = 0, r15
	;;
	(p7) add r8 = r8, r15
	(p6) sub r14 = r21, r14
	;;
	(p6) add r8 = r8, r14
	br.cloop.sptk.few .L144
	adds r24 = 1, r24
	add r23 = r23, r33
	;;
	cmp4.geu p6, p7 = 15, r24
	(p6) br.cond.dptk .L60
	mov ar.lc = r2
	br.ret.sptk.many b0
	.endp dev16_ia64#
