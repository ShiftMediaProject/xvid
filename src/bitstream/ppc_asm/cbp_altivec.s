#    Copyright (C) 2002 Guillaume Morin <guillaume@morinfr.org>, Alcôve
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#
#    $Id: cbp_altivec.s,v 1.1 2002-03-26 23:21:02 canard Exp $
#    $Source: /home/xvid/cvs_copy/cvs-server-root/xvid/xvidcore/src/bitstream/ppc_asm/cbp_altivec.s,v $
#    $Date: 2002-03-26 23:21:02 $
#    $Author: canard $
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
	# r9 will contain coeffs addr
	mr 9,3
	# r3 contains the result, therefore we set it to 0
	xor 3,3,3
	
	# CTR is the loop counter (rows)
	li 4,6
	mtctr 4	
	vxor 12,12,12
	lis 4,.skip@ha
	addi 4,4,.skip@l
	lvx 10,0,4
.loop:
	mr 6,9
	# coeffs is a matrix of 16 bits cells
	lvxl 1,0,6
	vand 1,1,10

	addi 6,6,16
	lvxl 2,0,6
	
	addi 6,6,16
	lvxl 3,0,6
	
	addi 6,6,16
	lvxl 4,0,6
	
	addi 6,6,16
	lvxl 5,0,6
	
	addi 6,6,16
	lvxl 6,0,6
	
	addi 6,6,16
	lvxl 7,0,6
	
	addi 6,6,16
	lvxl 8,0,6

	vor 1,2,1
	vor 1,3,1
	vor 1,4,1
	vor 1,5,1
	vor 1,6,1
	vor 1,7,1
	vor 1,8,1
	
	vcmpequw. 3,1,12
	bt 24,.newline
.cbp:
	mfctr 5
	subi 5,5,1
	li 4,1
	slw 4,4,5
	or 3,3,4
.newline:
	addi 9,9,128
	bdnz .loop
	blr
