#include "../portab.h"
#include "adapt_quant.h"

#include <stdlib.h>				/* free, malloc */

#define MAX(a,b)      (((a) > (b)) ? (a) : (b))
#define RDIFF(a,b)    ((int)(a+0.5)-(int)(b+0.5))

int
normalize_quantizer_field(float *in,
						  int *out,
						  int num,
						  int min_quant,
						  int max_quant)
{
	int i;
	int finished;

	do {
		finished = 1;
		for (i = 1; i < num; i++) {
			if (RDIFF(in[i], in[i - 1]) > 2) {
				in[i] -= (float) 0.5;
				finished = 0;
			} else if (RDIFF(in[i], in[i - 1]) < -2) {
				in[i - 1] -= (float) 0.5;
				finished = 0;
			}

			if (in[i] > max_quant) {
				in[i] = (float) max_quant;
				finished = 0;
			}
			if (in[i] < min_quant) {
				in[i] = (float) min_quant;
				finished = 0;
			}
			if (in[i - 1] > max_quant) {
				in[i - 1] = (float) max_quant;
				finished = 0;
			}
			if (in[i - 1] < min_quant) {
				in[i - 1] = (float) min_quant;
				finished = 0;
			}
		}
	} while (!finished);

	out[0] = 0;
	for (i = 1; i < num; i++)
		out[i] = RDIFF(in[i], in[i - 1]);

	return (int) (in[0] + 0.5);
}

int
adaptive_quantization(unsigned char *buf,
					  int stride,
					  int *intquant,
					  int framequant,
					  int min_quant,
					  int max_quant,
					  int mb_width,
					  int mb_height)	// no qstride because normalization
{
	int i, j, k, l;

	static float *quant;
	unsigned char *ptr;
	float *val;
	float global = 0., maxval = 0.;

	const float DarkThres = 0.25;
	const float DarkAmpl = 7.0;
	
	const float BrightThres = 4.0;
	const float BrightAmpl = 5.0;

	const char LowestVal = 10;

	const float GlobalBrightThres = 220.0;
	const float GlobalDarkThres = 20.0;
	float global_quant = 1.0;

	if (!quant)
		if (!(quant = (float *) malloc(mb_width * mb_height * sizeof(float))))
			return -1;

	val = (float *) malloc(mb_width * mb_height * sizeof(float));

	for (k = 0; k < mb_height; k++) {
		for (l = 0; l < mb_width; l++)	// do this for all macroblocks individually 
		{
			quant[k * mb_width + l] = (float) framequant;

			// calculate luminance-masking
			ptr = &buf[16 * k * stride + 16 * l];	// address of MB

			val[k * mb_width + l] = 0.;

			for (i = 0; i < 16; i++)
				for (j = 0; j < 16; j++)
				{
					if( ptr[i * stride + j] < LowestVal )
						ptr[i * stride + j] = 0;
					val[k * mb_width + l] += ptr[i * stride + j];
				}
			val[k * mb_width + l] /= 256.;
			global += val[k * mb_width + l];
			if( val[k * mb_width + l] > maxval )
				maxval = val[k * mb_width + l];
		}
	}

	global /= mb_width * mb_height;
	maxval /= global;
	if( global < GlobalDarkThres )
		global_quant *= -1.0;
	else if ( global < GlobalBrightThres )
		global_quant = 0.0;

	for (k = 0; k < mb_height; k++) {
		for (l = 0; l < mb_width; l++)	// do this for all macroblocks individually 
		{
			val[k * mb_width + l] /= global;
			if (val[k * mb_width + l] < DarkThres)
				quant[k * mb_width + l] += global_quant +
					DarkAmpl * (DarkThres - val[k * mb_width + l]) / DarkThres;
			else if (val[k * mb_width + l] > BrightThres)
				quant[k * mb_width + l] += global_quant +
					BrightAmpl * (val[k * mb_width + l] - BrightThres) / (maxval - BrightThres);
		}
	}
	free(val);
	return normalize_quantizer_field(quant, intquant, mb_width * mb_height,
									 min_quant, max_quant);
}
