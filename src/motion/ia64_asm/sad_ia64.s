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



	
	
	
	
	
.text
	.align 16
	.global dev16_ia64#
	.proc dev16_ia64#
.auto
dev16_ia64:
	// renamings for better readability
	stride = r18
	pfs = r19			//for saving previous function state
	cura0 = r20			//address of first 8-byte block of cur
	cura1 = r21			//address of second 8-byte block of cur
	mean0 = r22			//registers for calculating the sum in parallel
	mean1 = r23
	mean2 = r24
	mean3 = r25
	dev0 = r26			//same for the deviation
	dev1 = r27			
	dev2 = r28
	dev3 = r29
	
	.body
	alloc pfs = ar.pfs, 2, 38, 0, 40

	mov cura0  = in0
	mov stride = in1
	add cura1 = 8, cura0
	
	.rotr c[32], psad[8] 		// just using rotating registers to get an array ;-)

.explicit
{.mmi
	ld8 c[0] = [cura0], stride	// load them ...
	ld8 c[1] = [cura1], stride
	;; 
}	
{.mmi	
	ld8 c[2] = [cura0], stride
	ld8 c[3] = [cura1], stride
	;; 
}
{.mmi	
	ld8 c[4] = [cura0], stride
	ld8 c[5] = [cura1], stride
	;;
}
{.mmi
	ld8 c[6] = [cura0], stride
	ld8 c[7] = [cura1], stride
	;;
}
{.mmi	
	ld8 c[8] = [cura0], stride
	ld8 c[9] = [cura1], stride
	;;
}
{.mmi
	ld8 c[10] = [cura0], stride
	ld8 c[11] = [cura1], stride
	;;
}
{.mii
	ld8 c[12] = [cura0], stride
	psad1 mean0 = c[0], r0		// get the sum of them ...
	psad1 mean1 = c[1], r0
}
{.mmi
	ld8 c[13] = [cura1], stride
	;; 
	ld8 c[14] = [cura0], stride
	psad1 mean2 = c[2], r0
}
{.mii
	ld8 c[15] = [cura1], stride
	psad1 mean3 = c[3], r0
	;; 
	psad1 psad[0] = c[4], r0
}
{.mmi
	ld8 c[16] = [cura0], stride
	ld8 c[17] = [cura1], stride
	psad1 psad[1] = c[5], r0
	;;
}
{.mii	
	ld8 c[18] = [cura0], stride
	psad1 psad[2] = c[6], r0
	psad1 psad[3] = c[7], r0
}
{.mmi	
	ld8 c[19] = [cura1], stride
	;; 
	ld8 c[20] = [cura0], stride
	psad1 psad[4] = c[8], r0
}
{.mii
	ld8 c[21] = [cura1], stride
	psad1 psad[5] = c[9], r0
	;;
	add mean0 = mean0, psad[0]
}
{.mmi
	ld8 c[22] = [cura0], stride
	ld8 c[23] = [cura1], stride
	add mean1 = mean1, psad[1]
	;; 
}
{.mii
	ld8 c[24] = [cura0], stride
	psad1 psad[0] = c[10], r0
	psad1 psad[1] = c[11], r0
}
{.mmi
	ld8 c[25] = [cura1], stride
	;; 
	ld8 c[26] = [cura0], stride
	add mean2 = mean2, psad[2]
}
{.mii
	ld8 c[27] = [cura1], stride
	add mean3 = mean3, psad[3]
	;; 
	psad1 psad[2] = c[12], r0
}
{.mmi
	ld8 c[28] = [cura0], stride
	ld8 c[29] = [cura1], stride
	psad1 psad[3] = c[13], r0
	;; 
}
{.mii
	ld8 c[30] = [cura0]
	psad1 psad[6] = c[14], r0
	psad1 psad[7] = c[15], r0
}
{.mmi
	ld8 c[31] = [cura1]
	;; 
	add mean0 = mean0, psad[0]
	add mean1 = mean1, psad[1]
}
{.mii
	add mean2 = mean2, psad[4]
	add mean3 = mean3, psad[5]
	;;
	psad1 psad[0] = c[16], r0
}
{.mmi
	add mean0 = mean0, psad[2]
	add mean1 = mean1, psad[3]
	psad1 psad[1] = c[17], r0
	;;
}
{.mii
	add mean2 = mean2, psad[6]
	psad1 psad[2] = c[18], r0
	psad1 psad[3] = c[19], r0
}
{.mmi
	add mean3 = mean3, psad[7]
	;; 
	add mean0 = mean0, psad[0]
	psad1 psad[4] = c[20], r0
}
{.mii
	add mean1 = mean1, psad[1]
	psad1 psad[5] = c[21], r0
	;;
	psad1 psad[6] = c[22], r0
}
{.mmi
	add mean2 = mean2, psad[2]
	add mean3 = mean3, psad[3]
	psad1 psad[7] = c[23], r0
	;;
}
{.mii
	add mean0 = mean0, psad[4]
	psad1 psad[0] = c[24], r0
	psad1 psad[1] = c[25], r0
}
{.mmi
	add mean1 = mean1, psad[5]
	;;
	add mean2 = mean2, psad[6]
	psad1 psad[2] = c[26], r0
}
{.mii
	add mean3 = mean3, psad[7]
	psad1 psad[3] = c[27], r0
	;; 
	psad1 psad[4] = c[28], r0
}
{.mmi
	add mean0 = mean0, psad[0]
	add mean1 = mean1, psad[1]
	psad1 psad[5] = c[29], r0
	;;
}
{.mii
	add mean2 = mean2, psad[2]
	psad1 psad[6] = c[30], r0
	psad1 psad[7] = c[31], r0
}
{.mmi
	add mean3 = mean3, psad[3]
	;;
	add mean0 = mean0, psad[4]
	add mean1 = mean1, psad[5]
}
{.mbb
	add mean2 = mean2, mean3
	nop.b 1
	nop.b 1
	;;
}
{.mib
	add mean0 = mean0, psad[6]
	add mean1 = mean1, psad[7]
	nop.b 1
	;;
}
{.mib
	add mean0 = mean0, mean1
	// add mean2 = 127, mean2	// this could make our division more exact, but does not help much
	;;
}
{.mib
	add mean0 = mean0, mean2
	;;
}

{.mib
	shr.u mean0 = mean0, 8		// divide them ...
	;;
}
{.mib
	mux1 mean0 = mean0, @brcst
	;; 
}	
{.mii
	nop.m 0
	psad1 dev0 = c[0], mean0	// and do a sad again ...
	psad1 dev1 = c[1], mean0
}
{.mii
	nop.m 0
	psad1 dev2 = c[2], mean0
	psad1 dev3 = c[3], mean0
}
{.mii
	nop.m 0
	psad1 psad[0] = c[4], mean0
	psad1 psad[1] = c[5], mean0
}
{.mii
	nop.m 0
	psad1 psad[2] = c[6], mean0
	psad1 psad[3] = c[7], mean0
}
{.mii
	nop.m 0
	psad1 psad[4] = c[8], mean0
	psad1 psad[5] = c[9], mean0
	;; 
}
{.mii
	add dev0 = dev0, psad[0]
	psad1 psad[6] = c[10], mean0
	psad1 psad[7] = c[11], mean0
}
{.mmi
	add dev1 = dev1, psad[1]

	add dev2 = dev2, psad[2]
	psad1 psad[0] = c[12], mean0
}
{.mii
	add dev3 = dev3, psad[3]
	psad1 psad[1] = c[13], mean0
	;; 
	psad1 psad[2] = c[14], mean0
}
{.mmi
	add dev0 = dev0, psad[4]
	add dev1 = dev1, psad[5]
	psad1 psad[3] = c[15], mean0
}
{.mii
	add dev2 = dev2, psad[6]
	psad1 psad[4] = c[16], mean0
	psad1 psad[5] = c[17], mean0
}
{.mmi
	add dev3 = dev3, psad[7]
	;; 
	add dev0 = dev0, psad[0]
	psad1 psad[6] = c[18], mean0
}
{.mii
	add dev1 = dev1, psad[1]
	psad1 psad[7] = c[19], mean0
	
	psad1 psad[0] = c[20], mean0
}
{.mmi	
	add dev2 = dev2, psad[2]
	add dev3 = dev3, psad[3]
	psad1 psad[1] = c[21], mean0
	;;
}
{.mii
	add dev0 = dev0, psad[4]
	psad1 psad[2] = c[22], mean0
	psad1 psad[3] = c[23], mean0
}
{.mmi
	add dev1 = dev1, psad[5]

	add dev2 = dev2, psad[6]
	psad1 psad[4] = c[24], mean0
}
{.mii
	add dev3 = dev3, psad[7]
	psad1 psad[5] = c[25], mean0
	;; 
	psad1 psad[6] = c[26], mean0
}
{.mmi
	add dev0 = dev0, psad[0]
	add dev1 = dev1, psad[1]
	psad1 psad[7] = c[27], mean0
}
{.mii
	add dev2 = dev2, psad[2]
	psad1 psad[0] = c[28], mean0
	psad1 psad[1] = c[29], mean0
}
{.mmi
	add dev3 = dev3, psad[3]
	;;
	add dev0 = dev0, psad[4]
	psad1 psad[2] = c[30], mean0
}
{.mii
	add dev1 = dev1, psad[5]
	psad1 psad[3] = c[31], mean0
	;; 
	add dev2 = dev2, psad[6]
}
{.mmi
	add dev3 = dev3, psad[7]
	add dev0 = dev0, psad[0]
	add dev1 = dev1, psad[1]
	;;
}
{.mii
	add dev2 = dev2, psad[2]
	add dev3 = dev3, psad[3]
	add ret0 = dev0, dev1
	;; 
}
{.mib
	add dev2 = dev2, dev3
	nop.i 1
	nop.b 1
	;; 
}
{.mib
	add ret0 = ret0, dev2
	nop.i 1
	br.ret.sptk.many b0
}
	.endp dev16_ia64#
