/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	8bit<->16bit transfer
 *
 *	This program is an implementation of a part of one or more MPEG-4
 *	Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
 *	to use this software module in hardware or software products are
 *	advised that its use may infringe existing patents or copyrights, and
 *	any such use would be at such party's own risk.  The original
 *	developer of this software module and his/her company, and subsequent
 *	editors and their companies, will have no liability for use of this
 *	software or modifications or derivatives thereof.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

/**************************************************************************
 *
 *	History:
 *
 *	14.04.2002	added transfer_8to16sub2
 *	07.01.2002	merge functions from compensate; rename functions
 *	22.12.2001	transfer_8to8add16 limit fix
 *	07.11.2001	initial version; (c)2001 peter ross <pross@cs.rmit.edu.au>

 *************************************************************************/

TRANSFER_8TO16SUB_PTR  transfer_8to16sub;
TRANSFER_8TO16SUB2_PTR transfer_8to16sub2;
TRANSFER_16TO8ADD_PTR  transfer_16to8add;

// function pointers

/*****************************************************************************
 *
TRANSFER_8TO16SUB_PTR transfer_8to16sub;
 * to a 16 bit data array.
TRANSFER_16TO8ADD_PTR transfer_16to8add;
 * This is typically used during motion compensation, that's why some
 * functions also do the addition/substraction of another buffer during the
 * so called  transfer.
 *
 */
			dst[j * 8 + i] = (int16_t) src[j * stride + i];
		}
	}
}

/*
 * SRC - the source buffer
 * DST - the destination buffer
 *
 * Then the function does the 8->16 bit transfer and this serie of operations :
 *
 *    SRC (16bit) = SRC
 *    DST (8bit)  = max(min(SRC, 255), 0)
 */

			int16_t pixel = src[j * 8 + i];

			if (pixel < 0) {
				pixel = 0;
			} else if (pixel > 255) {
				pixel = 255;
			}
			dst[j * stride + i] = (uint8_t) pixel;
		}
	}
}




/*
 * C   - the current buffer
 * R   - the reference buffer
 * DCT - the dct coefficient buffer
 *
 * Then the function does the 8->16 bit transfer and this serie of operations :
 *
 *    R   (8bit)  = R
 *    C   (8bit)  = R
 *    DCT (16bit) = C - R
  perform motion compensation (and 8bit->16bit dct transfer)
  ... with write back
*/
		for (i = 0; i < 8; i++) {
			uint8_t c = cur[j * stride + i];
			uint8_t r = ref[j * stride + i];

			cur[j * stride + i] = r;
			dct[j * 8 + i] = (int16_t) c - (int16_t) r;
		}
	}
}


/*
 * C   - the current buffer
 * R1  - the 1st reference buffer
 * R2  - the 2nd reference buffer
 * DCT - the dct coefficient buffer
 *
 * Then the function does the 8->16 bit transfer and this serie of operations :
 *
 *    R1  (8bit) = R1
 *    R2  (8bit) = R2
  performs bi motion compensation (and 8bit->16bit dct transfer)
  .. but does not write back
*/
	for (j = 0; j < 8; j++) {
		for (i = 0; i < 8; i++) {
			uint8_t c = cur[j * stride + i];
			int r = (ref1[j * stride + i] + ref2[j * stride + i] + 1) / 2;

			if (r > 255) {
				r = 255;
			}
			//cur[j * stride + i] = r;
			dct[j * 8 + i] = (int16_t) c - (int16_t) r;
		}
	}
}


/*
 * SRC - the source buffer
 * DST - the destination buffer
 *
 * Then the function does the 16->8 bit transfer and this serie of operations :
 *
 *    SRC (16bit) = SRC
 *    DST (8bit)  = max(min(DST+SRC, 255), 0)
 */
			int16_t pixel = (int16_t) dst[j * stride + i] + src[j * 8 + i];

			if (pixel < 0) {
				pixel = 0;
			} else if (pixel > 255) {
				pixel = 255;
			}
			dst[j * stride + i] = (uint8_t) pixel;
		}
	}
}

/*
 * SRC - the source buffer
 * DST - the destination buffer
 *
 * Then the function does the 8->8 bit transfer and this serie of operations :
 *
 *    SRC (8bit) = SRC
 *    DST (8bit) = SRC
 */



			dst[j * stride + i] = src[j * stride + i];
		}
	}
}
