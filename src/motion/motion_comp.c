// 30.10.2002	corrected qpel chroma rounding
// 04.10.2002	added qpel support to MBMotionCompensation
// 01.05.2002   updated MBMotionCompensationBVOP
// 14.04.2002   bframe compensation

#include <stdio.h>

#include "../encoder.h"
#include "../utils/mbfunctions.h"
#include "../image/interpolate8x8.h"
#include "../image/reduced.h"
#include "../utils/timer.h"
#include "motion.h"

#ifndef ABS
#define ABS(X) (((X)>0)?(X):-(X))
#endif
#ifndef SIGN
#define SIGN(X) (((X)>0)?1:-1)
#endif

#ifndef RSHIFT
#define RSHIFT(a,b) ((a) > 0 ? ((a) + (1<<((b)-1)))>>(b) : ((a) + (1<<((b)-1))-1)>>(b))
#endif

/* assume b>0 */
#ifndef RDIV
#define RDIV(a,b) (((a)>0 ? (a) + ((b)>>1) : (a) - ((b)>>1))/(b))
#endif


/* This is borrowed from	decoder.c   */
static __inline int gmc_sanitize(int value, int quarterpel, int fcode)
{
	int length = 1 << (fcode+4);

//	if (quarterpel) value *= 2;

	if (value < -length)
		return -length;
	else if (value >= length)
		return length-1;
	else return value;
}

/* And this is borrowed from   bitstream.c  until we find a common solution */

static uint32_t __inline
log2bin(uint32_t value)
{
/* Changed by Chenm001 */
#if !defined(_MSC_VER)
	int n = 0;

	while (value) {
		value >>= 1;
		n++;
	}
	return n;
#else
	__asm {
		bsr eax, value
		inc eax
	}
#endif
}


static __inline void
compensate16x16_interpolate(int16_t * const dct_codes,
							uint8_t * const cur,
							const uint8_t * const ref,
							const uint8_t * const refh,
							const uint8_t * const refv,
							const uint8_t * const refhv,
							uint8_t * const tmp,
							uint32_t x,
							uint32_t y,
							const int32_t dx,
							const int32_t dy,
							const int32_t stride,
							const int quarterpel,
							const int reduced_resolution,
							const int32_t rounding)
{
	const uint8_t * ptr;

	if (!reduced_resolution) {

		if(quarterpel) {
			if ((dx&3) | (dy&3)) {
				interpolate16x16_quarterpel(tmp - y * stride - x,
											(uint8_t *) ref, tmp + 32,
											tmp + 64, tmp + 96, x, y, dx, dy, stride, rounding);
				ptr = tmp;
			} else ptr =  ref + (y + dy/4)*stride + x + dx/4; // fullpixel position

		} else ptr = get_ref(ref, refh, refv, refhv, x, y, 1, dx, dy, stride);

		transfer_8to16sub(dct_codes, cur + y * stride + x,
							ptr, stride);
		transfer_8to16sub(dct_codes+64, cur + y * stride + x + 8,
							ptr + 8, stride);
		transfer_8to16sub(dct_codes+128, cur + y * stride + x + 8*stride,
							ptr + 8*stride, stride);
		transfer_8to16sub(dct_codes+192, cur + y * stride + x + 8*stride+8,
							ptr + 8*stride + 8, stride);

	} else { //reduced_resolution

		x *= 2; y *= 2;

		ptr = get_ref(ref, refh, refv, refhv, x, y, 1, dx, dy, stride);

		filter_18x18_to_8x8(dct_codes, cur+y*stride + x, stride);
		filter_diff_18x18_to_8x8(dct_codes, ptr, stride);

		filter_18x18_to_8x8(dct_codes+64, cur+y*stride + x + 16, stride);
		filter_diff_18x18_to_8x8(dct_codes+64, ptr + 16, stride);

		filter_18x18_to_8x8(dct_codes+128, cur+(y+16)*stride + x, stride);
		filter_diff_18x18_to_8x8(dct_codes+128, ptr + 16*stride, stride);

		filter_18x18_to_8x8(dct_codes+192, cur+(y+16)*stride + x + 16, stride);
		filter_diff_18x18_to_8x8(dct_codes+192, ptr + 16*stride + 16, stride);

		transfer32x32_copy(cur + y*stride + x, ptr, stride);
	}
}

static __inline void
compensate8x8_interpolate(	int16_t * const dct_codes,
							uint8_t * const cur,
							const uint8_t * const ref,
							const uint8_t * const refh,
							const uint8_t * const refv,
							const uint8_t * const refhv,
							uint8_t * const tmp,
							uint32_t x,
							uint32_t y,
							const int32_t dx,
							const int32_t dy,
							const int32_t stride,
							const int32_t quarterpel,
							const int reduced_resolution,
							const int32_t rounding)
{
	const uint8_t * ptr;

	if (!reduced_resolution) {

		if(quarterpel) {
			if ((dx&3) | (dy&3)) {
				interpolate8x8_quarterpel(tmp - y*stride - x,
										(uint8_t *) ref, tmp + 32,
										tmp + 64, tmp + 96, x, y, dx, dy, stride, rounding);
				ptr = tmp;
			} else ptr = ref + (y + dy/4)*stride + x + dx/4; // fullpixel position
		} else ptr = get_ref(ref, refh, refv, refhv, x, y, 1, dx, dy, stride);

			transfer_8to16sub(dct_codes, cur + y * stride + x, ptr, stride);

	} else { //reduced_resolution

		x *= 2; y *= 2;

		ptr = get_ref(ref, refh, refv, refhv, x, y, 1, dx, dy, stride);

		filter_18x18_to_8x8(dct_codes, cur+y*stride + x, stride);
		filter_diff_18x18_to_8x8(dct_codes, ptr, stride);

		transfer16x16_copy(cur + y*stride + x, ptr, stride);
	}
}

/* XXX: slow, inelegant... */
static void
interpolate18x18_switch(uint8_t * const cur,
						const uint8_t * const refn,
						const uint32_t x,
						const uint32_t y,
						const int32_t dx,
						const int dy,
						const int32_t stride,
						const int32_t rounding)
{
	interpolate8x8_switch(cur, refn, x-1, y-1, dx, dy, stride, rounding);
	interpolate8x8_switch(cur, refn, x+7, y-1, dx, dy, stride, rounding);
	interpolate8x8_switch(cur, refn, x+9, y-1, dx, dy, stride, rounding);

	interpolate8x8_switch(cur, refn, x-1, y+7, dx, dy, stride, rounding);
	interpolate8x8_switch(cur, refn, x+7, y+7, dx, dy, stride, rounding);
	interpolate8x8_switch(cur, refn, x+9, y+7, dx, dy, stride, rounding);

	interpolate8x8_switch(cur, refn, x-1, y+9, dx, dy, stride, rounding);
	interpolate8x8_switch(cur, refn, x+7, y+9, dx, dy, stride, rounding);
	interpolate8x8_switch(cur, refn, x+9, y+9, dx, dy, stride, rounding);
}

static void
CompensateChroma(	int dx, int dy,
					const int i, const int j,
					IMAGE * const Cur,
					const IMAGE * const Ref,
					uint8_t * const temp,
					int16_t * const coeff,
					const int32_t stride,
					const int rounding,
					const int rrv)
{ /* uv-block-based compensation */

	if (!rrv) {
		transfer_8to16sub(coeff, Cur->u + 8 * j * stride + 8 * i,
							interpolate8x8_switch2(temp, Ref->u, 8 * i, 8 * j,
													dx, dy, stride, rounding),
							stride);
		transfer_8to16sub(coeff + 64, Cur->v + 8 * j * stride + 8 * i,
 							interpolate8x8_switch2(temp, Ref->v, 8 * i, 8 * j,
													dx, dy, stride, rounding),
							stride);
	} else {
		uint8_t * current, * reference;

		current = Cur->u + 16*j*stride + 16*i;
		reference = temp - 16*j*stride - 16*i;
		interpolate18x18_switch(reference, Ref->u, 16*i, 16*j, dx, dy, stride, rounding);
		filter_18x18_to_8x8(coeff, current, stride);
		filter_diff_18x18_to_8x8(coeff, temp, stride);
		transfer16x16_copy(current, temp, stride);

		current = Cur->v + 16*j*stride + 16*i;
		interpolate18x18_switch(reference, Ref->v, 16*i, 16*j, dx, dy, stride, rounding);
		filter_18x18_to_8x8(coeff + 64, current, stride);
		filter_diff_18x18_to_8x8(coeff + 64, temp, stride);
		transfer16x16_copy(current, temp, stride);
	}
}

void
MBMotionCompensation(MACROBLOCK * const mb,
					const uint32_t i,
					const uint32_t j,
					const IMAGE * const ref,
					const IMAGE * const refh,
					const IMAGE * const refv,
					const IMAGE * const refhv,
					const IMAGE * const refGMC,
					IMAGE * const cur,
					int16_t * dct_codes,
					const uint32_t width,
					const uint32_t height,
					const uint32_t edged_width,
					const int32_t quarterpel,
					const int reduced_resolution,
					const int32_t rounding)
{
	int32_t dx;
	int32_t dy;

	uint8_t * const tmp = refv->u;

	if ( (!reduced_resolution) && (mb->mode == MODE_NOT_CODED) ) {	/* quick copy for early SKIP */
/* early SKIP is only activated in P-VOPs, not in S-VOPs, so mcsel can never be 1 */

		transfer16x16_copy(cur->y + 16 * (i + j * edged_width),
						  ref->y + 16 * (i + j * edged_width),
						  edged_width);

		transfer8x8_copy(cur->u + 8 * (i + j * edged_width/2),
							ref->u + 8 * (i + j * edged_width/2),
							edged_width / 2);
		transfer8x8_copy(cur->v + 8 * (i + j * edged_width/2),
							ref->v + 8 * (i + j * edged_width/2),
							edged_width / 2);
		return;
	}

	if ((mb->mode == MODE_NOT_CODED || mb->mode == MODE_INTER
				|| mb->mode == MODE_INTER_Q)) {

	/* reduced resolution + GMC:  not possible */

		if (mb->mcsel) {

			/* call normal routine once, easier than "if (mcsel)"ing all the time */

			transfer_8to16sub(&dct_codes[0*64], cur->y + 16*j*edged_width + 16*i,
								 			refGMC->y + 16*j*edged_width + 16*i, edged_width);
			transfer_8to16sub(&dct_codes[1*64], cur->y + 16*j*edged_width + 16*i+8,
											refGMC->y + 16*j*edged_width + 16*i+8, edged_width);
			transfer_8to16sub(&dct_codes[2*64], cur->y + (16*j+8)*edged_width + 16*i,
											refGMC->y + (16*j+8)*edged_width + 16*i, edged_width);
			transfer_8to16sub(&dct_codes[3*64], cur->y + (16*j+8)*edged_width + 16*i+8,
											refGMC->y + (16*j+8)*edged_width + 16*i+8, edged_width);

/* lumi is needed earlier for mode decision, but chroma should be done block-based, but it isn't, yet. */

			transfer_8to16sub(&dct_codes[4 * 64], cur->u + 8 *j*edged_width/2 + 8*i,
								refGMC->u + 8 *j*edged_width/2 + 8*i, edged_width/2);

			transfer_8to16sub(&dct_codes[5 * 64], cur->v + 8*j* edged_width/2 + 8*i,
								refGMC->v + 8*j* edged_width/2 + 8*i, edged_width/2);

			return;
		}

		/* ordinary compensation */

		dx = (quarterpel ? mb->qmvs[0].x : mb->mvs[0].x);
		dy = (quarterpel ? mb->qmvs[0].y : mb->mvs[0].y);

		if (reduced_resolution) {
			dx = RRV_MV_SCALEUP(dx);
			dy = RRV_MV_SCALEUP(dy);
		}

		compensate16x16_interpolate(&dct_codes[0 * 64], cur->y, ref->y, refh->y,
							refv->y, refhv->y, tmp, 16 * i, 16 * j, dx, dy,
							edged_width, quarterpel, reduced_resolution, rounding);

		if (quarterpel) { dx /= 2; dy /= 2; }

		dx = (dx >> 1) + roundtab_79[dx & 0x3];
		dy = (dy >> 1) + roundtab_79[dy & 0x3];

	} else {					// mode == MODE_INTER4V
		int k, sumx = 0, sumy = 0;
		const VECTOR * const mvs = (quarterpel ? mb->qmvs : mb->mvs);

		for (k = 0; k < 4; k++) {
			dx = mvs[k].x;
			dy = mvs[k].y;
			sumx += quarterpel ? dx/2 : dx;
			sumy += quarterpel ? dy/2 : dy;

			if (reduced_resolution){
				dx = RRV_MV_SCALEUP(dx);
				dy = RRV_MV_SCALEUP(dy);
			}

			compensate8x8_interpolate(&dct_codes[k * 64], cur->y, ref->y, refh->y,
									refv->y, refhv->y, tmp, 16 * i + 8*(k&1), 16 * j + 8*(k>>1), dx,
									dy, edged_width, quarterpel, reduced_resolution, rounding);
		}
		dx = (sumx >> 3) + roundtab_76[sumx & 0xf];
		dy = (sumy >> 3) + roundtab_76[sumy & 0xf];
	}

	CompensateChroma(dx, dy, i, j, cur, ref, tmp,
					&dct_codes[4 * 64], edged_width / 2, rounding, reduced_resolution);
}


void
MBMotionCompensationBVOP(MBParam * pParam,
						MACROBLOCK * const mb,
						const uint32_t i,
						const uint32_t j,
						IMAGE * const cur,
						const IMAGE * const f_ref,
						const IMAGE * const f_refh,
						const IMAGE * const f_refv,
						const IMAGE * const f_refhv,
						const IMAGE * const b_ref,
						const IMAGE * const b_refh,
						const IMAGE * const b_refv,
						const IMAGE * const b_refhv,
						int16_t * dct_codes)
{
	const uint32_t edged_width = pParam->edged_width;
	int32_t dx, dy, b_dx, b_dy, sumx, sumy, b_sumx, b_sumy;
	int k;
	const int quarterpel = pParam->m_quarterpel;
	const uint8_t * ptr1, * ptr2;
	uint8_t * const tmp = f_refv->u;
	const VECTOR * const fmvs = (quarterpel ? mb->qmvs : mb->mvs);
	const VECTOR * const bmvs = (quarterpel ? mb->b_qmvs : mb->b_mvs);

	switch (mb->mode) {
	case MODE_FORWARD:
		dx = fmvs->x; dy = fmvs->y;

		compensate16x16_interpolate(&dct_codes[0 * 64], cur->y, f_ref->y, f_refh->y,
							f_refv->y, f_refhv->y, tmp, 16 * i, 16 * j, dx,
							dy, edged_width, quarterpel, 0, 0);

		if (quarterpel) { dx /= 2; dy /= 2; }

		CompensateChroma(	(dx >> 1) + roundtab_79[dx & 0x3],
							(dy >> 1) + roundtab_79[dy & 0x3],
							i, j, cur, f_ref, tmp,
							&dct_codes[4 * 64], edged_width / 2, 0, 0);

		return;

	case MODE_BACKWARD:
		b_dx = bmvs->x; b_dy = bmvs->y;

		compensate16x16_interpolate(&dct_codes[0 * 64], cur->y, b_ref->y, b_refh->y,
										b_refv->y, b_refhv->y, tmp, 16 * i, 16 * j, b_dx,
										b_dy, edged_width, quarterpel, 0, 0);

		if (quarterpel) { b_dx /= 2; b_dy /= 2; }

		CompensateChroma(	(b_dx >> 1) + roundtab_79[b_dx & 0x3],
							(b_dy >> 1) + roundtab_79[b_dy & 0x3],
							i, j, cur, b_ref, tmp,
							&dct_codes[4 * 64], edged_width / 2, 0, 0);

		return;

	case MODE_INTERPOLATE: /* _could_ use DIRECT, but would be overkill (no 4MV there) */
	case MODE_DIRECT_NO4V:
		dx = fmvs->x; dy = fmvs->y;
		b_dx = bmvs->x; b_dy = bmvs->y;

		if (quarterpel) {

			if ((dx&3) | (dy&3)) {
				interpolate16x16_quarterpel(tmp - i * 16 - j * 16 * edged_width,
					(uint8_t *) f_ref->y, tmp + 32,
					tmp + 64, tmp + 96, 16*i, 16*j, dx, dy, edged_width, 0);
				ptr1 = tmp;
			} else ptr1 = f_ref->y + (16*j + dy/4)*edged_width + 16*i + dx/4; // fullpixel position

			if ((b_dx&3) | (b_dy&3)) {
				interpolate16x16_quarterpel(tmp - i * 16 - j * 16 * edged_width + 16,
					(uint8_t *) b_ref->y, tmp + 32,
					tmp + 64, tmp + 96, 16*i, 16*j, b_dx, b_dy, edged_width, 0);
				ptr2 = tmp + 16;
			} else ptr2 = b_ref->y + (16*j + b_dy/4)*edged_width + 16*i + b_dx/4; // fullpixel position

			b_dx /= 2;
			b_dy /= 2;
			dx /= 2;
			dy /= 2;

		} else {
			ptr1 = get_ref(f_ref->y, f_refh->y, f_refv->y, f_refhv->y,
							i, j, 16, dx, dy, edged_width);

			ptr2 = get_ref(b_ref->y, b_refh->y, b_refv->y, b_refhv->y,
							i, j, 16, b_dx, b_dy, edged_width);
		}
		for (k = 0; k < 4; k++)
				transfer_8to16sub2(&dct_codes[k * 64],
									cur->y + (i * 16+(k&1)*8) + (j * 16+((k>>1)*8)) * edged_width,
									ptr1 + (k&1)*8 + (k>>1)*8*edged_width,
									ptr2 + (k&1)*8 + (k>>1)*8*edged_width, edged_width);


		dx = (dx >> 1) + roundtab_79[dx & 0x3];
		dy = (dy >> 1) + roundtab_79[dy & 0x3];

		b_dx = (b_dx >> 1) + roundtab_79[b_dx & 0x3];
		b_dy = (b_dy >> 1) + roundtab_79[b_dy & 0x3];

		break;

	default: // MODE_DIRECT (or MODE_DIRECT_NONE_MV in case of bframes decoding)
		sumx = sumy = b_sumx = b_sumy = 0;

		for (k = 0; k < 4; k++) {

			dx = fmvs[k].x; dy = fmvs[k].y;
			b_dx = bmvs[k].x; b_dy = bmvs[k].y;

			if (quarterpel) {
				sumx += dx/2; sumy += dy/2;
				b_sumx += b_dx/2; b_sumy += b_dy/2;

				if ((dx&3) | (dy&3)) {
					interpolate8x8_quarterpel(tmp - (i * 16+(k&1)*8) - (j * 16+((k>>1)*8)) * edged_width,
						(uint8_t *) f_ref->y,
						tmp + 32, tmp + 64, tmp + 96,
						16*i + (k&1)*8, 16*j + (k>>1)*8, dx, dy, edged_width, 0);
					ptr1 = tmp;
				} else ptr1 = f_ref->y + (16*j + (k>>1)*8 + dy/4)*edged_width + 16*i + (k&1)*8 + dx/4;

				if ((b_dx&3) | (b_dy&3)) {
					interpolate8x8_quarterpel(tmp - (i * 16+(k&1)*8) - (j * 16+((k>>1)*8)) * edged_width + 16,
						(uint8_t *) b_ref->y,
						tmp + 16, tmp + 32, tmp + 48,
						16*i + (k&1)*8, 16*j + (k>>1)*8, b_dx, b_dy, edged_width, 0);
					ptr2 = tmp + 16;
				} else ptr2 = b_ref->y + (16*j + (k>>1)*8 + b_dy/4)*edged_width + 16*i + (k&1)*8 + b_dx/4;
			} else {
				sumx += dx; sumy += dy;
				b_sumx += b_dx; b_sumy += b_dy;

				ptr1 = get_ref(f_ref->y, f_refh->y, f_refv->y, f_refhv->y,
								2*i + (k&1), 2*j + (k>>1), 8, dx, dy, edged_width);
				ptr2 = get_ref(b_ref->y, b_refh->y, b_refv->y, b_refhv->y,
								2*i + (k&1), 2*j + (k>>1), 8, b_dx, b_dy,  edged_width);
			}
			transfer_8to16sub2(&dct_codes[k * 64],
								cur->y + (i * 16+(k&1)*8) + (j * 16+((k>>1)*8)) * edged_width,
								ptr1, ptr2,	edged_width);

		}

		dx = (sumx >> 3) + roundtab_76[sumx & 0xf];
		dy = (sumy >> 3) + roundtab_76[sumy & 0xf];
		b_dx = (b_sumx >> 3) + roundtab_76[b_sumx & 0xf];
		b_dy = (b_sumy >> 3) + roundtab_76[b_sumy & 0xf];

		break;
	}

	// uv block-based chroma interpolation for direct and interpolate modes
	transfer_8to16sub2(&dct_codes[4 * 64],
						cur->u + (j * 8) * edged_width / 2 + (i * 8),
						interpolate8x8_switch2(tmp, b_ref->u, 8 * i, 8 * j,
												b_dx, b_dy, edged_width / 2, 0),
						interpolate8x8_switch2(tmp + 8, f_ref->u, 8 * i, 8 * j,
												dx, dy, edged_width / 2, 0),
						edged_width / 2);

	transfer_8to16sub2(&dct_codes[5 * 64],
						cur->v + (j * 8) * edged_width / 2 + (i * 8),
						interpolate8x8_switch2(tmp, b_ref->v, 8 * i, 8 * j,
												b_dx, b_dy, edged_width / 2, 0),
						interpolate8x8_switch2(tmp + 8, f_ref->v, 8 * i, 8 * j,
												dx, dy, edged_width / 2, 0),
						edged_width / 2);
}



void generate_GMCparameters( const int num_wp, const int res,
						const WARPPOINTS *const warp,
						const int width, const int height,
						GMC_DATA *const gmc)
{
	const int du0 = warp->duv[0].x;
	const int dv0 = warp->duv[0].y;
	const int du1 = warp->duv[1].x;
	const int dv1 = warp->duv[1].y;
	const int du2 = warp->duv[2].x;
	const int dv2 = warp->duv[2].y;

	gmc->W = width;
	gmc->H = height;

	gmc->rho = 4 - log2bin(res-1);  // = {3,2,1,0} for res={2,4,8,16}

	gmc->alpha = log2bin(gmc->W-1);
	gmc->Ws = (1 << gmc->alpha);

	gmc->dxF = 16*gmc->Ws + RDIV( 8*gmc->Ws*du1, gmc->W );
	gmc->dxG = RDIV( 8*gmc->Ws*dv1, gmc->W );
	gmc->Fo  = (res*du0 + 1) << (gmc->alpha+gmc->rho-1);
	gmc->Go  = (res*dv0 + 1) << (gmc->alpha+gmc->rho-1);

	if (num_wp==2) {
		gmc->dyF = -gmc->dxG;
		gmc->dyG =  gmc->dxF;
	} else if (num_wp==3) {
		gmc->beta = log2bin(gmc->H-1);
		gmc->Hs = (1 << gmc->beta);
		gmc->dyF =			 RDIV( 8*gmc->Hs*du2, gmc->H );
		gmc->dyG = 16*gmc->Hs + RDIV( 8*gmc->Hs*dv2, gmc->H );
		if (gmc->beta > gmc->alpha) {
			gmc->dxF <<= (gmc->beta - gmc->alpha);
			gmc->dxG <<= (gmc->beta - gmc->alpha);
			gmc->alpha = gmc->beta;
			gmc->Ws = 1<< gmc->beta;
		} else {
			gmc->dyF <<= gmc->alpha - gmc->beta;
			gmc->dyG <<= gmc->alpha - gmc->beta;
		}
	}

	gmc->cFo = gmc->dxF + gmc->dyF + (1 << (gmc->alpha+gmc->rho+1));
	gmc->cFo += 16*gmc->Ws*(du0-1);

	gmc->cGo = gmc->dxG + gmc->dyG + (1 << (gmc->alpha+gmc->rho+1));
	gmc->cGo += 16*gmc->Ws*(dv0-1);
}

void
generate_GMCimage(	const GMC_DATA *const gmc_data, // [input] precalculated data
					const IMAGE *const pRef,		// [input]
					const int mb_width,
					const int mb_height,
					const int stride,
					const int stride2,
					const int fcode, 				// [input] some parameters...
  					const int32_t quarterpel,		// [input] for rounding avgMV
					const int reduced_resolution,	// [input] ignored
					const int32_t rounding,			// [input] for rounding image data
					MACROBLOCK *const pMBs, 		// [output] average motion vectors
					IMAGE *const pGMC)				// [output] full warped image
{

	unsigned int mj,mi;
	VECTOR avgMV;

	for (mj = 0; mj < (unsigned int)mb_height; mj++)
		for (mi = 0; mi < (unsigned int)mb_width; mi++) {

			avgMV = generate_GMCimageMB(gmc_data, pRef, mi, mj,
						stride, stride2, quarterpel, rounding, pGMC);

			pMBs[mj*mb_width+mi].amv.x = gmc_sanitize(avgMV.x, quarterpel, fcode);
			pMBs[mj*mb_width+mi].amv.y = gmc_sanitize(avgMV.y, quarterpel, fcode);
			pMBs[mj*mb_width+mi].mcsel = 0; /* until mode decision */
	}
}



#define MLT(i)  (((16-(i))<<16) + (i))
static const uint32_t MTab[16] = {
  MLT( 0), MLT( 1), MLT( 2), MLT( 3), MLT( 4), MLT( 5), MLT( 6), MLT(7),
  MLT( 8), MLT( 9), MLT(10), MLT(11), MLT(12), MLT(13), MLT(14), MLT(15)
};
#undef MLT

VECTOR generate_GMCimageMB( const GMC_DATA *const gmc_data,
							const IMAGE *const pRef,
							const int mi, const int mj,
							const int stride,
							const int stride2,
							const int quarterpel,
							const int rounding,
							IMAGE *const pGMC)
{
	const int W = gmc_data->W;
	const int H = gmc_data->H;

	const int rho = gmc_data->rho;
	const int alpha = gmc_data->alpha;

	const int rounder = ( 128 - (rounding<<(rho+rho)) ) << 16;

	const int dxF = gmc_data->dxF;
	const int dyF = gmc_data->dyF;
	const int dxG = gmc_data->dxG;
	const int dyG = gmc_data->dyG;

	uint8_t *dstY, *dstU, *dstV;

	int I,J;
	VECTOR avgMV = {0,0};

	int32_t Fj, Gj;

	dstY = &pGMC->y[(mj*16)*stride+mi*16] + 16;

	Fj = gmc_data->Fo + dyF*mj*16 + dxF*mi*16;
	Gj = gmc_data->Go + dyG*mj*16 + dxG*mi*16;
	
	for (J = 16; J > 0; --J) {
		int32_t Fi, Gi;

		Fi = Fj; Fj += dyF;
		Gi = Gj; Gj += dyG;
		for (I = -16; I < 0; ++I) {
			int32_t F, G;
			uint32_t ri, rj;

			F = ( Fi >> (alpha+rho) ) << rho; Fi += dxF;
			G = ( Gi >> (alpha+rho) ) << rho; Gi += dxG;

			avgMV.x += F;
			avgMV.y += G;

			ri = MTab[F&15];
			rj = MTab[G&15];

			F >>= 4;
			G >>= 4;

			if (F < -1) F = -1;
			else if (F > W) F = W;
			if (G< -1) G=-1;
			else if (G>H) G=H;

			{	 // MMX-like bilinear...
				const int offset = G*stride + F;
				uint32_t f0, f1;
				f0 = pRef->y[ offset +0 ];
				f0 |= pRef->y[ offset +1 ] << 16;
				f1 = pRef->y[ offset+stride +0 ];
				f1 |= pRef->y[ offset+stride +1 ] << 16;
				f0 = (ri*f0)>>16;
				f1 = (ri*f1) & 0x0fff0000;
				f0 |= f1;
				f0 = ( rj*f0 + rounder ) >> 24;

				dstY[I] = (uint8_t)f0;
			}
		}
		
		dstY += stride;
	}

	dstU = &pGMC->u[(mj*8)*stride2+mi*8] + 8;
	dstV = &pGMC->v[(mj*8)*stride2+mi*8] + 8;

	Fj = gmc_data->cFo + dyF*4 *mj*8 + dxF*4 *mi*8;
	Gj = gmc_data->cGo + dyG*4 *mj*8 + dxG*4 *mi*8;
	
	for (J = 8; J > 0; --J) {
		int32_t Fi, Gi;
		Fi = Fj; Fj += 4*dyF;
		Gi = Gj; Gj += 4*dyG;

		for (I = -8; I < 0; ++I) {
			int32_t F, G;
			uint32_t ri, rj;

			F = ( Fi >> (alpha+rho+2) ) << rho; Fi += 4*dxF;
			G = ( Gi >> (alpha+rho+2) ) << rho; Gi += 4*dxG;

			ri = MTab[F&15];
			rj = MTab[G&15];

			F >>= 4;
			G >>= 4;

			if (F < -1) F=-1;
			else if (F >= W/2) F = W/2;
			if (G < -1) G = -1;
			else if (G >= H/2) G = H/2;

			{
				const int offset = G*stride2 + F;
				uint32_t f0, f1;

				f0	= pRef->u[ offset		 +0 ];
				f0 |= pRef->u[ offset		 +1 ] << 16;
				f1	= pRef->u[ offset+stride2 +0 ];
				f1 |= pRef->u[ offset+stride2 +1 ] << 16;
				f0 = (ri*f0)>>16;
				f1 = (ri*f1) & 0x0fff0000;
				f0 |= f1;
				f0 = ( rj*f0 + rounder ) >> 24;

				dstU[I] = (uint8_t)f0;


				f0	= pRef->v[ offset		 +0 ];
				f0 |= pRef->v[ offset		 +1 ] << 16;
				f1	= pRef->v[ offset+stride2 +0 ];
				f1 |= pRef->v[ offset+stride2 +1 ] << 16;
				f0 = (ri*f0)>>16;
				f1 = (ri*f1) & 0x0fff0000;
				f0 |= f1;
				f0 = ( rj*f0 + rounder ) >> 24;

				dstV[I] = (uint8_t)f0;
			}
		}
		dstU += stride2;
		dstV += stride2;
	}


	avgMV.x -= 16*((256*mi+120)<<4);	// 120 = 15*16/2
	avgMV.y -= 16*((256*mj+120)<<4);

	avgMV.x = RSHIFT( avgMV.x, (4+7-quarterpel) );
	avgMV.y = RSHIFT( avgMV.y, (4+7-quarterpel) );

	return avgMV;
}



#ifdef OLD_GRUEL_GMC
void
generate_GMCparameters(	const int num_wp,			// [input]: number of warppoints
						const int res, 			// [input]: resolution
						const WARPPOINTS *const warp, // [input]: warp points
						const int width, const int height,
						GMC_DATA *const gmc) 	// [output] precalculated parameters
{

/* We follow mainly two sources: The original standard, which is ugly, and the
   thesis from Andreas Dehnhardt, which is much nicer.

	Notation is: indices are written next to the variable,
				 primes in the standard are denoted by a suffix 'p'.
	types are   "c"=constant, "i"=input parameter, "f"=calculated, then fixed,
				"o"=output data, " "=other, "u" = unused, "p"=calc for every pixel

type | variable name  |   ISO name (TeX-style) |  value or range  |  usage
-------------------------------------------------------------------------------------
 c   | H			  |   H 				   |  [16 , ?]  	  |  image width (w/o edges)
 c   | W			  |   W 				   |  [16 , ?]  	  |  image height (w/o edges)

 c   | i0			  |   i_0				   |  0 			  |  ref. point #1, X
 c   | j0			  |   j_0				   |  0 			  |  ref. point #1, Y
 c   | i1			  |   i_1				   |  W 			  |  ref. point #2, X
 c   | j1			  |   j_1				   |  0 			  |  ref. point #2, Y
 cu  | i2			  |   i_2				   |  0 			  |  ref. point #3, X
 cu  | i2			  |   j_2				   |  H 			  |  ref. point #3, Y

 i   | du0  		  |   du[0] 			   |  [-16863,16863]  |  warp vector #1, Y
 i   | dv0  		  |   dv[0] 			   |  [-16863,16863]  |  warp vector #1, Y
 i   | du1  		  |   du[1] 			   |  [-16863,16863]  |  warp vector #2, Y
 i   | dv1  		  |   dv[1] 			   |  [-16863,16863]  |  warp vector #2, Y
 iu  | du2  		  |   du[2] 			   |  [-16863,16863]  |  warp vector #3, Y	
 iu  | dv2  		  |   dv[2] 			   |  [-16863,16863]  |  warp vector #3, Y

 i   | s			  |   s					|  {2,4,8,16}	  |  interpol. resolution
 f   | sigma		  |		-			   |  log2(s)		 |  X / s == X >> sigma
 f   | r			  |   r					|  =16/s		   |  complementary res.
 f   | rho			|   \rho				 |  log2(r)		 |  X / r == X >> rho

 f   | i0s			|   i'_0				 |				  |
 f   | j0s			|   j'_0				 |				  |
 f	 | i1s			|   i'_1				 |				  |
 f	 | j1s			|   j'_1				 |				  |
 f	 | i2s			|   i'_2				 |				  |
 f	 | j2s			|   j'_2				 |				  |

 f   | alpha		  |   \alpha			   |				  |  2^{alpha-1} < W <= 2^alpha
 f   | beta		   |   \beta				|				  |  2^{beta-1} < H <= 2^beta

 f   | Ws			 |   W'				   | W = 2^{alpha}	|  scaled width
 f   | Hs			 |   H'				   | W = 2^{beta}	 |  scaled height

 f   | i1ss		   |   i''_1				|  "virtual sprite stuff"
 f   | j1ss		   |   j''_1				|  "virtual sprite stuff"
 f   | i2ss		   |   i''_2				|  "virtual sprite stuff"
 f   | j2ss		   |   j''_2				|  "virtual sprite stuff"
*/

/* Some calculations are disabled because we only use 2 warppoints at the moment */

	int du0 = warp->duv[0].x;
	int dv0 = warp->duv[0].y;
	int du1 = warp->duv[1].x;
	int dv1 = warp->duv[1].y;
//	int du2 = warp->duv[2].x;
//	int dv2 = warp->duv[2].y;

	gmc->num_wp = num_wp;

	gmc->s = res;						/* scaling parameters 2,4,8 or 16 */
	gmc->sigma = log2bin(res-1);	/* log2bin(15)=4, log2bin(16)=5, log2bin(17)=5  */
	gmc->r = 16/res;
	gmc->rho = 4 - gmc->sigma; 		/* = log2bin(r-1) */

	gmc->W = width;
	gmc->H = height;			/* fixed reference coordinates */

	gmc->alpha = log2bin(gmc->W-1);
	gmc->Ws= 1<<gmc->alpha;

//	gmc->beta = log2bin(gmc->H-1);
//	gmc->Hs= 1<<gmc->beta;

//	printf("du0=%d dv0=%d du1=%d dv1=%d s=%d sigma=%d W=%d alpha=%d, Ws=%d, rho=%d\n",du0,dv0,du1,dv1,gmc->s,gmc->sigma,gmc->W,gmc->alpha,gmc->Ws,gmc->rho);

	/* i2s is only needed for num_wp >= 3, etc.  */
	/* the 's' values are in 1/s pel resolution */
	gmc->i0s = res/2 * ( du0 );
	gmc->j0s = res/2 * ( dv0 );
	gmc->i1s = res/2 * (2*width + du1 + du0 );
	gmc->j1s = res/2 * ( dv1 + dv0 );
//	gmc->i2s = res/2 * ( du2 + du0 );
//	gmc->j2s = res/2 * (2*height + dv2 + dv0 );

	/* i2s and i2ss are only needed for num_wp == 3, etc.  */

	/* the 'ss' values are in 1/16 pel resolution */
	gmc->i1ss = 16*gmc->Ws + ROUNDED_DIV(((gmc->W-gmc->Ws)*(gmc->r*gmc->i0s) + gmc->Ws*(gmc->r*gmc->i1s - 16*gmc->W)),gmc->W);
	gmc->j1ss = ROUNDED_DIV( ((gmc->W - gmc->Ws)*(gmc->r*gmc->j0s) + gmc->Ws*gmc->r*gmc->j1s) ,gmc->W );

//	gmc->i2ss = ROUNDED_DIV( ((gmc->H - gmc->Hs)*(gmc->r*gmc->i0s) + gmc->Hs*(gmc->r*gmc->i2s)), gmc->H);
//	gmc->j2ss = 16*gmc->Hs + ROUNDED_DIV( ((gmc->H-gmc->Hs)*(gmc->r*gmc->j0s) + gmc->Ws*(gmc->r*gmc->j2s - 16*gmc->H)), gmc->H);

	return;
}

void
generate_GMCimage(	const GMC_DATA *const gmc_data, 	// [input] precalculated data
					const IMAGE *const pRef,			// [input]
					const int mb_width,
					const int mb_height,
					const int stride,
					const int stride2,
					const int fcode, 					// [input] some parameters...
  					const int32_t quarterpel,			// [input] for rounding avgMV
					const int reduced_resolution,		// [input] ignored
					const int32_t rounding,			// [input] for rounding image data
					MACROBLOCK *const pMBs, 	// [output] average motion vectors
					IMAGE *const pGMC)			// [output] full warped image
{

	unsigned int mj,mi;
	VECTOR avgMV;

	for (mj = 0;mj < mb_height; mj++)
	for (mi = 0;mi < mb_width; mi++) {
		
		avgMV = generate_GMCimageMB(gmc_data, pRef, mi, mj,
					stride, stride2, quarterpel, rounding, pGMC);

		pMBs[mj*mb_width+mi].amv.x = gmc_sanitize(avgMV.x, quarterpel, fcode);
		pMBs[mj*mb_width+mi].amv.y = gmc_sanitize(avgMV.y, quarterpel, fcode);
		pMBs[mj*mb_width+mi].mcsel = 0; /* until mode decision */
	}
}


VECTOR generate_GMCimageMB(	const GMC_DATA *const gmc_data, /* [input] all precalc data */
							const IMAGE *const pRef,		/* [input] */
							const int mi, const int mj, 	/* [input] MB position  */
							const int stride,				/* [input] Lumi stride */
							const int stride2,				/* [input] chroma stride */
							const int quarterpel, 			/* [input] for rounding of avgMV */
							const int rounding, 			/* [input] for rounding of imgae data */
							IMAGE *const pGMC)				/* [outut] generate image */

/*
type | variable name  |   ISO name (TeX-style) |  value or range  |  usage
-------------------------------------------------------------------------------------
 p   | F			  |   F(i,j)			   |				  | pelwise motion vector X in s-th pel
 p   | G			  |   G(i,j)			   |				  | pelwise motion vector Y in s-th pel
 p   | Fc			 |   F_c(i,j)			 |				  |
 p   | Gc			 |   G_c(i,j)			 |				  | same for chroma

 p   | Y00			|   Y_{00}			   |  [0,255*s*s]	 | first: 4 neighbouring Y-values
 p   | Y01			|   Y_{01}			   |  [0,255]		 | at fullpel position, around the
 p   | Y10			|   Y_{10}			   |  [0,255*s]	   | position where pelweise MV points to
 p   | Y11			|   Y_{11}			   |  [0,255]		 | later: bilinear interpol Y-values in Y00

 p   | C00			|   C_{00}			   |  [0,255*s*s]	 | same for chroma Cb and Cr
 p   | C01			|   C_{01}			   |  [0,255]		 |
 p   | C10			|   C_{10}			   |  [0,255*s]	   |
 p   | C11			|   C_{11}			   |  [0,255]		 |

*/
{
	const int W = gmc_data->W;
	const int H = gmc_data->H;

	const int s = gmc_data->s;
	const int sigma = gmc_data->sigma;

	const int r = gmc_data->r;
	const int rho = gmc_data->rho;

	const int i0s = gmc_data->i0s;
	const int j0s = gmc_data->j0s;

	const int i1ss = gmc_data->i1ss;
	const int j1ss = gmc_data->j1ss;
//	const int i2ss = gmc_data->i2ss;
//	const int j2ss = gmc_data->j2ss;

	const int alpha = gmc_data->alpha;
	const int Ws	= gmc_data->Ws;

//	const int beta  = gmc_data->beta;
//	const int Hs	= gmc_data->Hs;

	int I,J;
	VECTOR avgMV = {0,0};

	for (J=16*mj;J<16*(mj+1);J++)
	for (I=16*mi;I<16*(mi+1);I++)
	{
		int F= i0s + ( ((-r*i0s+i1ss)*I + (r*j0s-j1ss)*J + (1<<(alpha+rho-1))) >>  (alpha+rho) );
		int G= j0s + ( ((-r*j0s+j1ss)*I + (-r*i0s+i1ss)*J + (1<<(alpha+rho-1))) >> (alpha+rho) );

/* this naive implementation (with lots of multiplications) isn't slower (rather faster) than
   working incremental. Don't ask me why... maybe the whole this is memory bound? */

		const int ri= F & (s-1); // fractional part of pelwise MV X
		const int rj= G & (s-1); // fractional part of pelwise MV Y

		int Y00,Y01,Y10,Y11;

/* unclipped values are used for avgMV */
		avgMV.x += F-(I<<sigma);		/* shift position to 1/s-pel, as the MV is */
		avgMV.y += G-(J<<sigma);		/* TODO: don't do this (of course) */

		F >>= sigma;
		G >>= sigma;

/* clip values to be in range. Since we have edges, clip to 1 less than lower boundary
   this way positions F+1/G+1 are still right */

		if (F< -1)
			F=-1;
		else if (F>W)
			F=W;	/* W or W-1 doesn't matter, so save 1 subtract ;-) */
		if (G< -1)
			G=-1;
		else if (G>H)
			G=H;		/* dito */

		Y00 = pRef->y[ G*stride + F ];				// Lumi values
		Y01 = pRef->y[ G*stride + F+1 ];
		Y10 = pRef->y[ G*stride + F+stride ];
		Y11 = pRef->y[ G*stride + F+stride+1 ];

		/* bilinear interpolation */
		Y00 = ((s-ri)*Y00 + ri*Y01);
		Y10 = ((s-ri)*Y10 + ri*Y11);
		Y00 = ((s-rj)*Y00 + rj*Y10 + s*s/2 - rounding ) >> (sigma+sigma);

		pGMC->y[J*stride+I] = (uint8_t)Y00;										/* output 1 Y-pixel */
	}


/* doing chroma _here_ is even more stupid and slow, because won't be used until Compensation and
	most likely not even then (only if the block really _is_ GMC)
*/

	for (J=8*mj;J<8*(mj+1);J++)		/* this plays the role of j_c,i_c in the standard */
	for (I=8*mi;I<8*(mi+1);I++)		/* For I_c we have to use I_c = 4*i_c+1 ! */
	{
		/* same positions for both chroma components, U=Cb and V=Cr */
		int Fc=((-r*i0s+i1ss)*(4*I+1) + (r*j0s-j1ss)*(4*J+1) +2*Ws*r*i0s
						-16*Ws +(1<<(alpha+rho+1)))>>(alpha+rho+2);
	 	int Gc=((-r*j0s+j1ss)*(4*I+1) +(-r*i0s+i1ss)*(4*J+1) +2*Ws*r*j0s
						-16*Ws +(1<<(alpha+rho+1))) >>(alpha+rho+2);

		const int ri= Fc & (s-1); // fractional part of pelwise MV X
		const int rj= Gc & (s-1); // fractional part of pelwise MV Y

		int C00,C01,C10,C11;

		Fc >>= sigma;
		Gc >>= sigma;

		if (Fc< -1)
			Fc=-1;
		else if (Fc>=W/2)
			Fc=W/2;		/* W or W-1 doesn't matter, so save 1 subtraction ;-) */
		if (Gc< -1)
			Gc=-1;
		else if (Gc>=H/2)
			Gc=H/2;		/* dito */

/* now calculate U data */
		C00 = pRef->u[ Gc*stride2 + Fc ];				// chroma-value Cb
		C01 = pRef->u[ Gc*stride2 + Fc+1 ];
		C10 = pRef->u[ (Gc+1)*stride2 + Fc ];
		C11 = pRef->u[ (Gc+1)*stride2 + Fc+1 ];

		/* bilinear interpolation */
		C00 = ((s-ri)*C00 + ri*C01);
		C10 = ((s-ri)*C10 + ri*C11);
		C00 = ((s-rj)*C00 + rj*C10 + s*s/2 - rounding ) >> (sigma+sigma);

		pGMC->u[J*stride2+I] = (uint8_t)C00;										/* output 1 U-pixel */

/* now calculate V data */
		C00 = pRef->v[ Gc*stride2 + Fc ];				// chroma-value Cr
		C01 = pRef->v[ Gc*stride2 + Fc+1 ];
		C10 = pRef->v[ (Gc+1)*stride2 + Fc ];
		C11 = pRef->v[ (Gc+1)*stride2 + Fc+1 ];

		/* bilinear interpolation */
		C00 = ((s-ri)*C00 + ri*C01);
		C10 = ((s-ri)*C10 + ri*C11);
		C00 = ((s-rj)*C00 + rj*C10 + s*s/2 - rounding ) >> (sigma+sigma);

		pGMC->v[J*stride2+I] = (uint8_t)C00;										/* output 1 V-pixel */
	}

/* The average vector is rounded from 1/s-pel to 1/2 or 1/4 using the '//' operator*/

	avgMV.x = RSHIFT( avgMV.x, (sigma+7-quarterpel) );
	avgMV.y = RSHIFT( avgMV.y, (sigma+7-quarterpel) );

	/* ^^^^ this is the way MS Reference Software does it */

	return avgMV;	/* clipping to fcode area is done outside! */
}

#endif
