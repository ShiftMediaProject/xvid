//   ------------------------------------------------------------------------------
//   *
//   * Optimized Assembler Versions of sad8 and sad16
//   *
//   ------------------------------------------------------------------------------
//   *
//   * Hannes Jütting and Christopher Özbek 
//   * {s_juetti,s_oezbek}@ira.uka.de
//   *
//   * Programmed for the IA64 laboratory held at University Karlsruhe 2002
//   * http://www.info.uni-karlsruhe.de/~rubino/ia64p/
//   *
//   ------------------------------------------------------------------------------
//   *
//   * These are the optimized assembler versions of sad8 and sad16, which calculate 
//   * the sum of absolute differences between two 8x8/16x16 block matrices. 
//   *
//   * Our approach uses:
//   *   - The Itanium command psad1, which solves the problem in hardware. 
//   *   - Modulo-Scheduled Loops as the best way to loop unrolling on the IA64 
//   *     EPIC architecture
//   *   - Alignment resolving to avoid memory faults
//   *
//   ------------------------------------------------------------------------------

	.text

//   ------------------------------------------------------------------------------
//   * SAD16_IA64
//   *
//   *  In:
//   *    r32 = cur	(aligned)
//   *    r33 = ref	(not aligned)
//   *    r34 = stride
//   *    r35 = bestsad
//   *  Out:
//   *    r8 = sum of absolute differences
//   *
//   ------------------------------------------------------------------------------
		
	.align 16
	.global sad16_ia64#
	.proc sad16_ia64#
sad16_ia64:

	
	// Define Latencies
	LL16=3 // load  latency
	SL16=1 // shift latency
	OL16=1 // or    latency
	PL16=1 // psad  latency
	AL16=1 // add   latency

	// Allocate Registern in RSE
	alloc r9=ar.pfs,4,36,0,40

	// lfetch [r32]			// might help
		
	mov r8 = r0			// clear the return reg

	// Save LC and predicates
	mov r20 = ar.lc			
	mov r21 = pr			

	dep.z r23	= r33, 3, 3	// get the # of bits ref is misaligned
	and r15		= -8, r33	// align the ref pointer by deleting the last 3 bit

	mov r14		= r32		// save the cur pointer
	mov r16		= r34		// save stride
	mov r17		= r35		// save bestsad
	
	;;
	add r18		= 8, r14	// precalc second cur pointer
	add r19		= 8, r15	// precalc second ref pointer
	add r27		= 16, r15	// precalc third  ref pointer
	sub r25		= 64, r23	// # of right shifts

	// Initialize Loop-counters 
	mov ar.lc = 15			// loop 16 times
	mov ar.ec = LL16 + SL16 + OL16 + PL16 + AL16 + AL16	
	mov pr.rot = 1 << 16		// reseting rotating predicate regs and set p16 to 1
	;;
	
	// Intialize Arrays for Register Rotation
	.rotr r_cur_ld1[LL16+SL16+OL16+1], r_cur_ld2[LL16+SL16+OL16+1], r_ref_16_ld1[LL16+1], r_ref_16_ld2[LL16+1], r_ref_16_ld3[LL16+1], r_ref_16_shru1[SL16], r_ref_16_shl1[SL16], r_ref_16_shru2[SL16], r_ref_16_shl2[SL16+1], r_ref_16_or1[OL16], r_ref_16_or2[OL16+1], r_psad1[PL16+1], r_psad2[PL16+1], r_add_16[AL16+1]
	.rotp p_ld_16[LL16], p_sh_16[SL16], p_or_16[OL16], p_psad_16[PL16], p_add1_16[AL16], p_add2_16[AL16]
	
.L_loop16:
	{.mmi
		(p_ld_16[0]) ld8 r_cur_ld1[0] = [r14], r16				// Cur load first 8 Byte
		(p_ld_16[0]) ld8 r_cur_ld2[0] = [r18], r16				// Cur load next 8 Byte
		(p_psad_16[0]) psad1 r_psad1[0] = r_cur_ld1[LL16+SL16+OL16], r_ref_16_or2[0]	// psad of cur and ref
	}
	{.mmi
		(p_ld_16[0]) ld8 r_ref_16_ld1[0] = [r15], r16				// Ref load first 8 Byte (unaligned)
		(p_ld_16[0]) ld8 r_ref_16_ld2[0] = [r19], r16				// Ref load next 8 Byte (unaligned)
		(p_psad_16[0]) psad1 r_psad2[0] = r_cur_ld2[LL16+SL16+OL16], r_ref_16_or2[OL16]	// psad of cur_2 and ref_2
	}
	{.mii
		(p_ld_16[0]) ld8 r_ref_16_ld3[0] = [r27], r16				// Ref load third 8 Byte (unaligned)	
		(p_or_16[0]) or r_ref_16_or1[0]  = r_ref_16_shl1[0], r_ref_16_shru2[0]  // Ref or r_ref_16_shl1 + 1 and r_ref_16_shl1 + 1
		(p_sh_16[0]) shr.u r_ref_16_shru1[0] = r_ref_16_ld1[LL16], r23		// Ref shift
	}
	{.mii
		(p_or_16[0]) or r_ref_16_or2[0]  = r_ref_16_shl2[0], r_ref_16_shl2[SL16]	// Ref or r_ref_shru2 + 1 and r_ref_shl2 + 1
		(p_sh_16[0]) shl r_ref_16_shl1[0] = r_ref_16_ld2[LL16], r25		// Ref shift
		(p_sh_16[0]) shr.u r_ref_16_shru2[0] = r_ref_16_ld2[LL16], r23		// Ref shift
	}
	{.mib
	    	(p_add2_16[0]) cmp.ge.unc p6, p7 = r8, r17
		(p_sh_16[0]) shl r_ref_16_shl2[0]= r_ref_16_ld3[LL16], r25		// Ref shift
	    	(p6) br.spnt.few .L_loop_exit16
	}
	{.mmb
		(p_add1_16[0]) add r_add_16[0] = r_psad1[PL16], r_psad2[PL16]		// add the psad results
		(p_add2_16[0]) add r8 = r8, r_add_16[AL16]				// add the results to the sum
		br.ctop.sptk.few .L_loop16
		;;
	}
.L_loop_exit16:

	// Restore LC and predicates
	mov ar.lc = r20			
	mov pr = r21,-1			
 	
 	// Return 
 	br.ret.sptk.many rp
	.endp sad16_ia64#

//   ------------------------------------------------------------------------------
//   * SAD8_IA64
//   *
//   *  In:
//   *    r32 = cur	(aligned)
//   *    r33 = ref	(not aligned)
//   *    r34 = stride
//   *  Out:
//   *    r8 = sum of absolute differences
//   *
//   ------------------------------------------------------------------------------
	
  	.align 16
	.global sad8_ia64#
	.proc sad8_ia64#

sad8_ia64:

	
	// Define Latencies
	LL8=3 // load  latency
	SL8=1 // shift latency
	OL8=1 // or    latency
	PL8=1 // psad  latency
	AL8=1 // add   latency
	
	// Allocate Registers in RSE
	alloc r9	= ar.pfs,3,21,0,24
	
	// lfetch [r32]				// Maybe this helps?
	
	mov r8		= r0			// Initialize result	
		
	mov r14		= r32			// Save Cur
	and r15		= -8, r33		// Align the Ref pointer by deleting the last 3 bit
	mov r16		= r34			// Save Stride	
	
	// Save LC and predicates
	mov r20		= ar.lc
	mov r21		= pr

	dep.z r23	= r33, 3, 3		// get the # of bits ref is misaligned
	
	;;
	
	add r19		= 8, r15		// Precalculate second load-offset
	sub r25		= 64, r23		// Precalculate # of shifts

	// Initialize Loop-Counters 
	mov ar.lc = 7				// Loop 7 times
	mov ar.ec = LL8 + SL8 + OL8 + PL8 + AL8	// Epiloque 
	mov pr.rot = 1 << 16			// Reset Predicate Registers and initialize with P16

	// Initalize Arrays for Register Rotation
	.rotr r_cur_ld[LL8+SL8+OL8+1], r_ref_ld1[LL8+1], r_ref_ld2[LL8+1], r_shru[SL8+1], r_shl[SL8+1], r_or[OL8+1], r_psad[PL8+1]
	.rotp p_ld[LL8], p_sh[SL8], p_or[OL8], p_psad[PL8], p_add[AL8]
	
	;;
.L_loop8:
//	{.mmi
		(p_ld[0]) ld8 r_ref_ld1[0]	= [r15], r16			// Load 1st 8Byte from Ref
		(p_ld[0]) ld8 r_cur_ld[0] 	= [r14], r16			// Load Cur 
		(p_psad[0]) psad1 r_psad[0]	= r_cur_ld[LL8+SL8+OL8], r_or[OL8]	// Do the Calculation
//	}
//	{.mii
		(p_ld[0]) ld8 r_ref_ld2[0]	= [r19], r16			// Load 2nd 8Byte from Ref 
		(p_sh[0]) shr.u r_shru[0]	= r_ref_ld1[LL8], r23		// Shift unaligned Ref parts
		(p_sh[0]) shl	r_shl[0] 	= r_ref_ld2[LL8], r25		// Shift unaligned Ref parts
//	}
//	{.mib
		(p_or[0]) or r_or[0] 		= r_shru[SL8], r_shl[SL8]	// Combine unaligned Ref parts
		(p_add[0]) add r8 		= r8, r_psad[PL8]		// Sum psad result
		br.ctop.sptk.few .L_loop8
		;; 
//	} 

	// Restore Loop counters
	mov ar.lc = r20
	mov pr = r21,-1
	
	// Return 
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
