/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Aligned memory allocator -
 *
 *  Copyright(C) 2002 Michael Militzer <isibaar@xvid.org>
 *               2002 Edouard Gomez
 *
 *  This file is part of XviD, a free MPEG-4 video encoder/decoder
 *
 *  XviD is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
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
 *  Under section 8 of the GNU General Public License, the copyright
 *  holders of XVID explicitly forbid distribution in the following
 *  countries:
 *
 *    - Japan
 *    - United States of America
 *
 *  Linking XviD statically or dynamically with other modules is making a
 *  combined work based on XviD.  Thus, the terms and conditions of the
 *  GNU General Public License cover the whole combination.
 *
 *  As a special exception, the copyright holders of XviD give you
 *  permission to link XviD with independent modules that communicate with
 *  XviD solely through the VFW1.1 and DShow interfaces, regardless of the
 *  license terms of these independent modules, and to copy and distribute
 *  the resulting combined work under terms of your choice, provided that
 *  every copy of the combined work is accompanied by a complete copy of
 *  the source code of XviD (the version of XviD used to produce the
 *  combined work), being distributed under the terms of the GNU General
 *  Public License plus this exception.  An independent module is a module
 *  which is not derived from or based on XviD.
 *
 *  Note that people who make modified versions of XviD are not obligated
 *  to grant this special exception for their modified versions; it is
 *  their choice whether to do so.  The GNU General Public License gives
 *  permission to release a modified version without this exception; this
 *  exception also makes it possible to release a modified version which
 *  carries forward this exception.
 *
 * $Id: mem_align.c,v 1.11 2002-11-17 00:51:11 edgomez Exp $
 *
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "mem_align.h"

/*****************************************************************************
 * xvid_malloc
 *
 * This function allocates 'size' bytes (usable by the user) on the heap and
 * takes care of the requested 'alignment'.
 * In order to align the allocated memory block, the xvid_malloc allocates
 * 'size' bytes + 'alignment' bytes. So try to keep alignment very small
 * when allocating small pieces of memory.
 *
 * NB : a block allocated by xvid_malloc _must_ be freed with xvid_free
 *      (the libc free will return an error)
 *
 * Returned value : - NULL on error
 *                  - Pointer to the allocated aligned block
 *
 ****************************************************************************/

void *
xvid_malloc(size_t size,
			uint8_t alignment)
{
	uint8_t *mem_ptr;

	if (!alignment) {

		/* We have not to satisfy any alignment */
		if ((mem_ptr = (uint8_t *) malloc(size + 1)) != NULL) {

			/* Store (mem_ptr - "real allocated memory") in *(mem_ptr-1) */
			*mem_ptr = 0;

			/* Return the mem_ptr pointer */
			return (void *) mem_ptr++;

		}

	} else {
		uint8_t *tmp;

		/*
		 * Allocate the required size memory + alignment so we
		 * can realign the data if necessary
		 */

		if ((tmp = (uint8_t *) malloc(size + alignment)) != NULL) {

			/* Align the tmp pointer */
			mem_ptr =
				(uint8_t *) ((ptr_t) (tmp + alignment - 1) &
							 (~(ptr_t) (alignment - 1)));

			/*
			 * Special case where malloc have already satisfied the alignment
			 * We must add alignment to mem_ptr because we must store
			 * (mem_ptr - tmp) in *(mem_ptr-1)
			 * If we do not add alignment to mem_ptr then *(mem_ptr-1) points
			 * to a forbidden memory space
			 */
			if (mem_ptr == tmp)
				mem_ptr += alignment;

			/*
			 * (mem_ptr - tmp) is stored in *(mem_ptr-1) so we are able to retrieve
			 * the real malloc block allocated and free it in xvid_free
			 */
			*(mem_ptr - 1) = (uint8_t) (mem_ptr - tmp);

			/* Return the aligned pointer */
			return (void *) mem_ptr;

		}
	}

	return NULL;

}

/*****************************************************************************
 * xvid_free
 *
 * Free a previously 'xvid_malloc' allocated block. Does not free NULL
 * references.
 *
 * Returned value : None.
 *
 ****************************************************************************/

void
xvid_free(void *mem_ptr)
{

	/* *(mem_ptr - 1) give us the offset to the real malloc block */
	if (mem_ptr)
		free((uint8_t *) mem_ptr - *((uint8_t *) mem_ptr - 1));

}
