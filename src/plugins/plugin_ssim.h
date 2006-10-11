
/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - SSIM plugin: computes the SSIM metric  -
 *
 *  Copyright(C) 2005 Johannes Reinhardt <Johannes.Reinhardt@gmx.de>
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
 *
 *
 ****************************************************************************/

#ifndef SSIM_H
#define SSIM_H

typedef struct{
	/*stat output*/
	int b_printstat;
	char* stat_path;
	
	/*visualize*/
	int b_visualize;

} plg_ssim_param_t;


int plugin_ssim(void * handle, int opt, void * param1, void * param2);

#endif
