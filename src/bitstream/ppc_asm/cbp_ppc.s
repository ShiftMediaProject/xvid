#    Copyright (C) 2002 Guillaume Morin, Alcôve
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
#    $Id: cbp_ppc.s,v 1.1 2002-03-21 23:42:53 canard Exp $
#    $Source: /home/xvid/cvs_copy/cvs-server-root/xvid/xvidcore/src/bitstream/ppc_asm/cbp_ppc.s,v $
#    $Date: 2002-03-21 23:42:53 $
#    $Author: canard $

#uint32_t calc_cbp_c(const int16_t codes[6][64])
#{
#    uint32_t i, j;
#    uint32_t cbp = 0;
#
#    for (i = 0; i < 6; i++) {
#		for (j = 1; j < 64/4; j+=4) {
#			if (codes[i][j]  |codes[i][j+1]|
#				codes[i][j+2]|codes[i][j+3]) {
#				cbp |= 1 << (5 - i);
#				break;
#			}
#		}
#    }
#    return cbp;
#}

.text
.global calc_cbp_ppc
calc_cbp_ppc:
	mr 9,3
	li 8,5
	xor 3,3,3
.loop:
	li 7,14
	mr 6,9
.loop2:
	lha 4,2(6)
	lha 5,4(6)
	or 4,5,4
	lha 5,6(6)
	or 4,5,4
	lha 5,8(6)
	or 4,5,4
	cmpwi 4,0
	bne .cbp
	addi 6,6,8
	subi 7,7,1
	cmpwi 7,-1
	beq .lastcoeffs
	b .loop2
.lastcoeffs:
	lha 4,2(6)
	lha 5,4(6)
	or 4,5,4
	lha 5,6(6)
	or 4,5,4
	cmpwi 4,0
	beq .newline
.cbp:
	li 4,1
	slw 4,4,8
	or 3,3,4
	b .newline
.newline:
	addi 9,9,128
	subi 8,8,1
	cmpwi 8,-1
	bne .loop
.end:
	blr
