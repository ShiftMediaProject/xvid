#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <stdlib.h>

#include "../portab.h"
#include "../global.h"
#include "colorspace.h"
#include "../xvid.h"

#define EDGE_SIZE  64

void init_image(uint32_t cpu_flags);


static void __inline
image_null(IMAGE * image)
{
	image->y = image->u = image->v = NULL;
}

int32_t image_create(IMAGE * image,
					 uint32_t edged_width,
					 uint32_t edged_height);
void image_destroy(IMAGE * image,
				   uint32_t edged_width,
				   uint32_t edged_height);

void image_swap(IMAGE * image1,
				IMAGE * image2);
				
void image_copy(IMAGE * image1,
				IMAGE * image2,
				uint32_t edged_width,
				uint32_t height);
				
void image_setedges(IMAGE * image,
					uint32_t edged_width,
					uint32_t edged_height,
					uint32_t width,
					uint32_t height);
					
void image_interpolate(const IMAGE * refn,
					   IMAGE * refh,
					   IMAGE * refv,
					   IMAGE * refhv,
					   uint32_t edged_width,
					   uint32_t edged_height,
					   uint32_t quarterpel,
					   uint32_t rounding);

float image_psnr(IMAGE * orig_image,
				 IMAGE * recon_image,
				 uint16_t stride,
				 uint16_t width,
				 uint16_t height);


float sse_to_PSNR(long sse, int pixels);

long plane_sse(uint8_t * orig,
		   uint8_t * recon,
		   uint16_t stride,
		   uint16_t width,
		   uint16_t height);

void
image_chroma_optimize(IMAGE * img, int width, int height, int edged_width);


int image_input(IMAGE * image,
				uint32_t width,
				int height,
				uint32_t edged_width,
				uint8_t * src,
				int src_stride,
				int csp,
				int interlaced);

int image_output(IMAGE * image,
				 uint32_t width,
				 int height,
				 uint32_t edged_width,
				 uint8_t * dst,
				 uint32_t dst_stride,
				 int csp,
				 int interlaced);



int image_dump_yuvpgm(const IMAGE * image,
					  const uint32_t edged_width,
					  const uint32_t width,
					  const uint32_t height,
					  char *filename);

float image_mad(const IMAGE * img1,
				const IMAGE * img2,
				uint32_t stride,
				uint32_t width,
				uint32_t height);

void
output_slice(IMAGE * cur, int edged_width, int width, XVID_DEC_PICTURE* out_frm, int mbx, int mby,int mbl);


void
image_clear(IMAGE * img, int width, int height, int edged_width,
					int y, int u, int v);


void
image_deblock_rrv(IMAGE * img, int edgeg_width,
				const MACROBLOCK * mbs, int mb_width, int mb_height, int mb_stride,
				int block, int flags);


#endif							/* _IMAGE_H_ */
