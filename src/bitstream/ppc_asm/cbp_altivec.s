#    Copyright (C) 2002 Guillaume Morin <guillaume@morinfr.org>, Alcôve
#
# *  This file is part of XviD, a free MPEG-4 video encoder/decoder
# *
# *  XviD is free software; you can redistribute it and/or modify it
# *  under the terms of the GNU General Public License as published by
# *  the Free Software Foundation; either version 2 of the License, or
# *  (at your option) any later version.
# *
# *  This program is distributed in the hope that it will be useful,
# *  but WITHOUT ANY WARRANTY; without even the implied warranty of
# *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# *  GNU General Public License for more details.
# *
# *  You should have received a copy of the GNU General Public License
# *  along with this program; if not, write to the Free Software
# *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
# *
# *  Under section 8 of the GNU General Public License, the copyright
# *  holders of XVID explicitly forbid distribution in the following
# *  countries:
# *
# *    - Japan
# *    - United States of America
# *
# *  Linking XviD statically or dynamically with other modules is making a
# *  combined work based on XviD.  Thus, the terms and conditions of the
# *  GNU General Public License cover the whole combination.
# *
# *  As a special exception, the copyright holders of XviD give you
# *  permission to link XviD with independent modules that communicate with
# *  XviD solely through the VFW1.1 and DShow interfaces, regardless of the
# *  license terms of these independent modules, and to copy and distribute
# *  the resulting combined work under terms of your choice, provided that
# *  every copy of the combined work is accompanied by a complete copy of
# *  the source code of XviD (the version of XviD used to produce the
# *  combined work), being distributed under the terms of the GNU General
# *  Public License plus this exception.  An independent module is a module
# *  which is not derived from or based on XviD.
# *
# *  Note that people who make modified versions of XviD are not obligated
# *  to grant this special exception for their modified versions; it is
# *  their choice whether to do so.  The GNU General Public License gives
# *  permission to release a modified version without this exception; this
# *  exception also makes it possible to release a modified version which
# *  carries forward this exception.
# *
#
#    $Id: cbp_altivec.s,v 1.3 2002-11-17 00:57:57 edgomez Exp $
#    $Source: /home/xvid/cvs_copy/cvs-server-root/xvid/xvidcore/src/bitstream/ppc_asm/cbp_altivec.s,v $
#    $Date: 2002-11-17 00:57:57 $
#    $Author: edgomez $
#
#    This is my first PPC ASM attempt. So I might do nasty things.
#    Please send any comments to <guillaume@morinfr.org>


# Returns a field of bits that indicates non zero ac blocks
# for this macro block
#
# uint32_t calc_cbp_c(const int16_t codes[6][64])
#{
#	uint32_t i, j;
#	uint32_t cbp = 0;
#
#	for (i = 0; i < 6; i++) {
#		for (j = 1; j < 61; j+=4) {
#			if (codes[i][j]  |codes[i][j+1]|
#			    codes[i][j+2]|codes[i][j+3]) {
#				cbp |= 1 << (5 - i);
#				break;
#			}
#		}
#
#		if(codes[i][j]|codes[i][j+1]|codes[i][j+2])
#			cbp |= 1 << (5 - i);
#
#	}
#
#	return cbp;
#
#}

.data
.skip:
	.quad 0x0000FFFFFFFFFFFF,-1
.align 4

.text
.global calc_cbp_altivec
calc_cbp_altivec:
	# Set VRSAVE
	li %r4,0xFFFFFFFF
	mtspr 256,%r4	

	# r9 will contain coeffs addr
	mr %r9,%r3
	# r3 contains the result, therefore we set it to 0
	li %r3,0
	
	# CTR is the loop counter (rows)
	li %r4,6
	mtctr %r4	

	# VR9 contains 0
	vxor 9,9,9

	# VR10 will help us to remove the first 16 bits of each row
	lis %r4,.skip@ha
	addi %r4,4,.skip@l
	lvx 10,0,%r4
.loop:
	mr %r6,%r9

	lvxl 1,0,%r6
	# Set the first 16 bits to 0
	vand 1,1,10

	addi %r6,%r6,16
	lvxl 2,0,6
	
	addi %r6,%r6,16
	lvxl 3,0,6
	
	addi %r6,%r6,16
	lvxl 4,0,6
	
	addi %r6,%r6,16
	lvxl 5,0,6
	
	addi %r6,%r6,16
	lvxl 6,0,6
	
	addi %r6,%r6,16
	lvxl 7,0,6
	
	addi %r6,%r6,16
	lvxl 8,0,6

	vor 1,2,1
	vor 1,3,1
	vor 1,4,1
	vor 1,5,1
	vor 1,6,1
	vor 1,7,1
	vor 1,8,1
	
	# is VR1 == 0
	vcmpequw. 3,1,9
	bt 24,.newline
.cbp:
	# cbp calc
	mfctr %r5
	subi %r5,%r5,1
	li %r4,1
	slw %r4,%r4,%r5
	or %r3,%r3,%r4
.newline:
	addi %r9,%r9,128
	bdnz .loop
	blr
