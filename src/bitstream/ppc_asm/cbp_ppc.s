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
#    $Id: cbp_ppc.s,v 1.8 2002-03-27 15:06:14 canard Exp $
#    $Source: /home/xvid/cvs_copy/cvs-server-root/xvid/xvidcore/src/bitstream/ppc_asm/cbp_ppc.s,v $
#    $Date: 2002-03-27 15:06:14 $
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
	.word 0,-1
.align 4 

.text
.global calc_cbp_ppc
calc_cbp_ppc:
	# r9 will contain coeffs addr
	mr 9,3
	# r8 is the loop counter (rows)
	li 8,5
	# r3 contains the result, therefore we set it to 0
	li 3,0
.loop:
	# CTR is the loop2 counter
	li 4,16
	mtctr 4
	# r6 is coeff pointer for this line
	mr 6,9
	lis 7,.skip@ha
	addi 7,7,.skip@l
	lwz 7,0(7)
.loop2:
	# coeffs is a matrix of 16 bits cells
	lwz 4,0(6)
	and 4,4,7
	li 7,-1

	lwz 5,4(6)
	# or. updates CR0
	or. 4,5,4
	# testing bit 2 (is zero) of CR0
	bf 2,.cbp
	addi 6,6,8
	bdnz .loop2
	b .newline
.cbp:
	li 4,1
	slw 4,4,8
	or 3,3,4
.newline:
	addi 9,9,128
	# updates CR0, blabla
	subic. 8,8,1
	bf 0,.loop
	blr
