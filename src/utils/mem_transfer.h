/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - 8<->16 bit buffer transfer header -
 *
 *  This program is an implementation of a part of one or more MPEG-4
 *  Video tools as specified in ISO/IEC 14496-2 standard.  Those intending
 *  to use this software module in hardware or software products are
 *  advised that its use may infringe existing patents or copyrights, and
 *  any such use would be at such party's own risk.  The original
 *  developer of this software module and his/her company, and subsequent
 *  editors and their companies, will have no liability for use of this
 *  software or modifications or derivatives thereof.
 *
 *  This program is free software ; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation ; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY ; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program ; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 ****************************************************************************/
/*****************************************************************************
 *
 *  History
 *
 *  - Sun Jun 16 00:12:49 2002 Added legal header
 *                             Cosmetic
 *  $Id: mem_transfer.h,v 1.6 2002-06-15 22:28:32 edgomez Exp $
 *
 ****************************************************************************/


#ifndef _MEM_TRANSFER_H
#define _MEM_TRANSFER_H

/*****************************************************************************
 * transfer8to16 API
 ****************************************************************************/

typedef void (TRANSFER_8TO16COPY) (int16_t * const dst,
								   const uint8_t * const src,
								   uint32_t stride);

typedef TRANSFER_8TO16COPY *TRANSFER_8TO16COPY_PTR;

/* Our global function pointer - Initialized in xvid.c */
extern TRANSFER_8TO16COPY_PTR transfer_8to16copy;

/* Implemented functions */
TRANSFER_8TO16COPY transfer_8to16copy_c;
TRANSFER_8TO16COPY transfer_8to16copy_mmx;
TRANSFER_8TO16COPY transfer_8to16copy_ia64;

/*****************************************************************************
 * transfer16to8 API
 ****************************************************************************/

typedef void (TRANSFER_16TO8COPY) (uint8_t * const dst,
								   const int16_t * const src,
								   uint32_t stride);
typedef TRANSFER_16TO8COPY *TRANSFER_16TO8COPY_PTR;

/* Our global function pointer - Initialized in xvid.c */
extern TRANSFER_16TO8COPY_PTR transfer_16to8copy;

/* Implemented functions */
TRANSFER_16TO8COPY transfer_16to8copy_c;
TRANSFER_16TO8COPY transfer_16to8copy_mmx;
TRANSFER_16TO8COPY transfer_16to8copy_ia64;

/*****************************************************************************
 * transfer8to16 + substraction op API
 ****************************************************************************/

typedef void (TRANSFER_8TO16SUB) (int16_t * const dct,
								  uint8_t * const cur,
								  const uint8_t * ref,
								  const uint32_t stride);

typedef TRANSFER_8TO16SUB *TRANSFER_8TO16SUB_PTR;

/* Our global function pointer - Initialized in xvid.c */
extern TRANSFER_8TO16SUB_PTR transfer_8to16sub;

/* Implemented functions */
TRANSFER_8TO16SUB transfer_8to16sub_c;
TRANSFER_8TO16SUB transfer_8to16sub_mmx;
TRANSFER_8TO16SUB transfer_8to16sub_ia64;

/*****************************************************************************
 * transfer8to16 + substraction op API - Bidirectionnal Version
 ****************************************************************************/

typedef void (TRANSFER_8TO16SUB2) (int16_t * const dct,
								   uint8_t * const cur,
								   const uint8_t * ref1,
								   const uint8_t * ref2,
								   const uint32_t stride);

typedef TRANSFER_8TO16SUB2 *TRANSFER_8TO16SUB2_PTR;

/* Our global function pointer - Initialized in xvid.c */
extern TRANSFER_8TO16SUB2_PTR transfer_8to16sub2;

/* Implemented functions */
TRANSFER_8TO16SUB2 transfer_8to16sub2_c;
//TRANSFER_8TO16SUB2 transfer_8to16sub2_mmx;
TRANSFER_8TO16SUB2 transfer_8to16sub2_ia64;


/*****************************************************************************
 * transfer16to8 + addition op API
 ****************************************************************************/

typedef void (TRANSFER_16TO8ADD) (uint8_t * const dst,
								  const int16_t * const src,
								  uint32_t stride);

typedef TRANSFER_16TO8ADD *TRANSFER_16TO8ADD_PTR;

/* Our global function pointer - Initialized in xvid.c */
extern TRANSFER_16TO8ADD_PTR transfer_16to8add;

/* Implemented functions */
TRANSFER_16TO8ADD transfer_16to8add_c;
TRANSFER_16TO8ADD transfer_16to8add_mmx;
TRANSFER_16TO8ADD transfer_16to8add_ia64;

/*****************************************************************************
 * transfer8to8 + no op
 ****************************************************************************/

typedef void (TRANSFER8X8_COPY) (uint8_t * const dst,
								 const uint8_t * const src,
								 const uint32_t stride);

typedef TRANSFER8X8_COPY *TRANSFER8X8_COPY_PTR;

/* Our global function pointer - Initialized in xvid.c */
extern TRANSFER8X8_COPY_PTR transfer8x8_copy;

/* Implemented functions */
TRANSFER8X8_COPY transfer8x8_copy_c;
TRANSFER8X8_COPY transfer8x8_copy_mmx;
TRANSFER8X8_COPY transfer8x8_copy_ia64;

#endif
