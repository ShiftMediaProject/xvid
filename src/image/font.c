/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - font module 
 *
 *  Copyright(C) 2002 Peter Ross <pross@xvid.org>
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
 ****************************************************************************/


#include "image.h"
#include "font.h"

#define FONT_WIDTH	4
#define FONT_HEIGHT	6


static const char ascii33[33][FONT_WIDTH*FONT_HEIGHT] = { 
	{	0,0,1,0,	// !
		0,0,1,0,
		0,0,1,0,
		0,0,1,0,
		0,0,0,0,
		0,0,1,0	},
	
	{	0,1,0,1,	// "
		0,1,0,1,
		0,0,0,0,
		0,0,0,0,
		0,0,0,0,
		0,0,0,0	},
	
	{	0,1,1,0,	// #
		1,1,1,1,
		0,1,1,0,
		0,1,1,0,
		1,1,1,1,
		0,1,1,0	},

	{	0,1,1,0,	// $
		1,0,1,1,
		1,1,1,0,
		0,1,1,1,
		1,1,0,1,
		0,1,1,0	},

	{	1,1,0,1,	// %
		1,0,0,1,
		0,0,1,0,
		0,1,0,0,
		1,0,0,1,
		1,0,1,1	},

	{	0,1,1,0,	//&
		1,0,0,0,
		0,1,0,1,
		1,0,1,0,
		1,0,1,0,
		0,1,0,1	},

	{	0,0,1,0,	// '
		0,0,1,0,
		0,0,0,0,
		0,0,0,0,
		0,0,0,0,
		0,0,0,0	},

	{	0,0,1,0,	// (
		0,1,0,0,
		0,1,0,0,
		0,1,0,0,
		0,1,0,0,
		0,0,1,0	},

	{	0,1,0,0,	// )
		0,0,1,0,
		0,0,1,0,
		0,0,1,0,
		0,0,1,0,
		0,1,0,0	},

	{	0,0,0,0,		// *
		1,0,0,1,
		0,1,1,0,
		1,1,1,1,
		0,1,1,0,
		1,0,0,1 },
	
	{	0,0,0,0,	// +
		0,0,1,0,
		0,0,1,0,
		0,1,1,1,
		0,0,1,0,
		0,0,1,0	},
	
	{	0,0,0,0,	// ,
		0,0,0,0,
		0,0,0,0,
		0,0,0,0,
		0,1,1,0,
		0,0,1,0	},
	
	{	0,0,0,0,	// -
		0,0,0,0,
		0,0,0,0,
		1,1,1,1,
		0,0,0,0,
		0,0,0,0	},

	{	0,0,0,0,	// .
		0,0,0,0,
		0,0,0,0,
		0,0,0,0,
		0,1,1,0,
		0,1,1,0	},

	{	0,0,0,1,	// /
		0,0,0,1,
		0,0,1,0,
		0,1,0,0,
		1,0,0,0,
		1,0,0,0	},

	{	0,1,1,0,		// 0
		1,0,0,1,
		1,0,1,1,
		1,1,0,1,
		1,0,0,1,
		0,1,1,0 },
	
	{	0,0,1,0,	// 1
		0,0,1,0,
		0,0,1,0,
		0,0,1,0,
		0,0,1,0,
		0,0,1,0	},
	
	{	0,1,1,0,	// 2
		1,0,0,1,
		0,0,1,0,
		0,1,0,0,
		1,0,0,0,
		1,1,1,1	},
	
	{	0,1,1,0,	// 3
		1,0,0,1,
		0,0,1,0,
		0,0,0,1,
		1,0,0,1,
		0,1,1,0	},

	{	0,0,1,0,	// 4
		0,1,1,0,
		1,0,1,0,
		1,1,1,1,
		0,0,1,0,
		0,0,1,0	},

	{	1,1,1,1,	// 5
		1,0,0,0,
		1,1,1,0,
		0,0,0,1,
		1,0,0,1,
		0,1,1,0	},

	{	0,1,1,1,	//6
		1,0,0,0,
		1,1,1,0,
		1,0,0,1,
		1,0,0,1,
		0,1,1,0	},

	{	1,1,1,0,	// 7
		0,0,0,1,
		0,0,0,1,
		0,0,1,0,
		0,0,1,0,
		0,0,1,0	},

	{	0,1,1,0,	// 8
		1,0,0,1,
		0,1,1,0,
		1,0,0,1,
		1,0,0,1,
		0,1,1,0	},

	{	0,1,1,0,	// 9
		1,0,0,1,
		1,0,0,1,
		0,1,1,1,
		0,0,0,1,
		1,1,1,0	},

	{	0,0,0,0,		// :
		0,0,0,0,
		0,0,1,0,
		0,0,0,0,
		0,0,1,0,
		0,0,0,0 },

	{	0,0,0,0,		// ;
		0,0,1,0,
		0,0,0,0,
		0,0,0,0,
		0,1,1,0,
		0,0,1,0 },

	{	0,0,0,1,		// <
		0,0,1,0,
		0,1,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1 },

	{	0,0,0,0,		// =
		1,1,1,1,
		0,0,0,0,
		0,0,0,0,
		1,1,1,1,
		0,0,0,0 },

	{	0,1,0,0,		// >
		0,0,1,0,
		0,0,0,1,
		0,0,0,1,
		0,0,1,0,
		0,1,0,0 },

	{	0,1,1,0,		// ?
		1,0,0,1,
		0,0,1,0,
		0,0,1,0,
		0,0,0,0,
		0,0,1,0 },

	{	0,1,1,0,		// @
		1,0,0,1,
		1,0,1,1,
		1,0,1,1,
		1,0,0,0,
		0,1,1,0 },

};


static const char ascii65[26][FONT_WIDTH*FONT_HEIGHT] = { 
	{	0,1,1,0,	// A
		1,0,0,1,
		1,0,0,1,
		1,1,1,1,
		1,0,0,1,
		1,0,0,1 },
	
	{	1,1,1,0,	// B
		1,0,0,1,
		1,1,1,0,
		1,0,0,1,
		1,0,0,1,
		1,1,1,0	},
	
	{	0,1,1,0,	// C
		1,0,0,1,
		1,0,0,0,
		1,0,0,0,
		1,0,0,1,
		0,1,1,0	},
	
	{	1,1,0,0,	// D
		1,0,1,0,
		1,0,0,1,
		1,0,0,1,
		1,0,1,0,
		1,1,0,0	},

	{	1,1,1,1,	// E
		1,0,0,0,
		1,1,1,0,
		1,0,0,0,
		1,0,0,0,
		1,1,1,1	},

	{	1,1,1,1,	// F
		1,0,0,0,
		1,1,1,0,
		1,0,0,0,
		1,0,0,0,
		1,0,0,0	},

	{	0,1,1,1,	// G
		1,0,0,0,
		1,0,1,1,
		1,0,0,1,
		1,0,0,1,
		0,1,1,0	},

	{	1,0,0,1,	// H
		1,0,0,1,
		1,1,1,1,
		1,0,0,1,
		1,0,0,1,
		1,0,0,1	},

	{	0,1,1,1,	// I
		0,0,1,0,
		0,0,1,0,
		0,0,1,0,
		0,0,1,0,
		0,1,1,1	},

	{	0,1,1,1,	// J
		0,0,1,0,
		0,0,1,0,
		0,0,1,0,
		1,0,1,0,
		0,1,0,0	},

	{	1,0,0,1,	// K
		1,0,0,1,
		1,1,1,0,
		1,0,0,1,
		1,0,0,1,
		1,0,0,1	},

	{	1,0,0,0,	// L
		1,0,0,0,
		1,0,0,0,
		1,0,0,0,
		1,0,0,0,
		1,1,1,1	},

	{	1,0,0,1,	// M
		1,1,1,1,
		1,1,1,1,
		1,0,0,1,
		1,0,0,1,
		1,0,0,1	},

	{	1,0,0,1,	// N
		1,1,0,1,
		1,1,0,1,
		1,0,1,1,
		1,0,1,1,
		1,0,0,1	},

	{	0,1,1,0,		// 0
		1,0,0,1,
		1,0,0,1,
		1,0,0,1,
		1,0,0,1,
		0,1,1,0 },

	{	1,1,1,0,		// P
		1,0,0,1,
		1,1,1,0,
		1,0,0,0,
		1,0,0,0,
		1,0,0,0 },

	{	0,1,1,0,		// Q
		1,0,0,1,
		1,0,0,1,
		1,0,0,1,
		1,0,1,0,
		0,1,0,1 },


	{	1,1,1,0,		// R
		1,0,0,1,
		1,1,1,0,
		1,0,0,1,
		1,0,0,1,
		1,0,0,1 },

	{	0,1,1,0,		// S
		1,0,0,1,
		0,1,0,0,
		0,0,1,0,
		1,0,0,1,
		0,1,1,0 },

	{	0,1,1,1,	// T
		0,0,1,0,
		0,0,1,0,
		0,0,1,0,
		0,0,1,0,
		0,0,1,0	},

	{	1,0,0,1,		// U
		1,0,0,1,
		1,0,0,1,
		1,0,0,1,
		1,0,0,1,
		1,1,1,1 },

	{	1,0,0,1,		// V
		1,0,0,1,
		1,0,0,1,
		0,1,1,0,
		0,1,1,0,
		0,1,1,0 },

	{	1,0,0,1,	// W
		1,0,0,1,
		1,0,0,1,
		1,1,1,1,
		1,1,1,1,
		1,0,0,1	},

	{	1,0,0,1,		// X
		1,0,0,1,
		0,1,1,0,
		1,0,0,1,
		1,0,0,1,
		1,0,0,1 },

	{	1,0,0,1,		// Y
		1,0,0,1,
		0,1,0,0,
		0,0,1,0,
		0,1,0,0,
		1,0,0,0 },

	{	1,1,1,1,		// Z
		0,0,0,1,
		0,0,1,0,
		0,1,0,0,
		1,0,0,0,
		1,1,1,1 },

};



static const char ascii91[6][FONT_WIDTH*FONT_HEIGHT] = { 
	{	0,1,1,0,		// [
		0,1,0,0,
		0,1,0,0,
		0,1,0,0,
		0,1,0,0,
		0,1,1,0 },

	{	1,0,0,0,		// '\'
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1,
		0,0,0,1 },

	{	0,1,1,0,		// ]
		0,0,1,0,
		0,0,1,0,
		0,0,1,0,
		0,1,1,0,

		0,0,1,0 },
	{	0,1,0,1,		// ^
		0,0,0,0,
		0,0,0,0,
		0,0,0,0,
		0,0,0,0,
		0,0,0,0 },

	{	0,0,0,0,		// _
		0,0,0,0,
		0,0,0,0,
		0,0,0,0,
		0,0,0,0,
		1,1,1,1 },

	{	0,1,0,0,		// `
		0,0,1,0,
		0,0,0,0,
		0,0,0,0,
		0,0,0,0,
		0,0,0,0 }
};


#define FONT_ZOOM	4

void draw_num(IMAGE * img, const int stride, const int height,
			  const char * font, const int x, const int y)
{
	int i, j;

	for (j = 0; j < FONT_ZOOM * FONT_HEIGHT && y+j < height; j++)
		for (i = 0; i < FONT_ZOOM * FONT_WIDTH && x+i < stride; i++)
			if (font[(j/FONT_ZOOM)*FONT_WIDTH + (i/FONT_ZOOM)])
			{
				int offset = (y+j)*stride + (x+i);
				int offset2 =((y+j)/2)*(stride/2) + ((x+i)/2);
				img->y[offset] = 255;
				img->u[offset2] = 127;
				img->v[offset2] = 127;
			}
}


#define FONT_BUF_SZ  1024

void image_printf(IMAGE * img, int edged_width, int height, 
				  int x, int y, char *fmt, ...)
{
	va_list args;
	char buf[FONT_BUF_SZ];
	size_t i;

	va_start(args, fmt);
	vsprintf(buf, fmt, args);

	for (i = 0; i < strlen(buf); i++) {
		const char * font;

		if (buf[i] >= '!' && buf[i] <= '@')
			font = ascii33[buf[i]-'!'];
		else if (buf[i] >= 'A' && buf[i] <= 'Z')
			font = ascii65[buf[i]-'A'];
		else if (buf[i] >= '[' && buf[i] <= '`')
			font = ascii91[buf[i]-'['];
		else if (buf[i] >= 'a' && buf[i] <= 'z')
			font = ascii65[buf[i]-'a'];
		else
			continue;

		draw_num(img, edged_width, height, font, x + i*FONT_ZOOM*(FONT_WIDTH+1), y);
	}
}
