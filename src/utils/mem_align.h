/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Memory alignment header file -
 *
 *  Copyright(C) 2002 Michael Militzer
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
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: mem_align.h,v 1.4 2002-09-05 20:59:45 edgomez Exp $
 *
 ****************************************************************************/

#ifndef _MEM_ALIGN_H_
#define _MEM_ALIGN_H_

#include "../portab.h"

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

void *xvid_malloc(size_t size,
				  uint8_t alignment);

void xvid_free(void *mem_ptr);


#endif /* _MEM_ALIGN_H_ */
