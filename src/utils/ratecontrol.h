#ifndef _RATECONTROL_H_
#define _RATECONTROL_H_

#include "../encoder.h"
#include "../portab.h"

void RateControlInit(uint32_t target_rate, uint32_t rc_buffersize, int framerate,
					 int width, int height, int max_quant, int min_quant);

int RateControlGetQ(int keyframe);

void RateControlUpdate(int16_t quant, int frame_size, int keyframe);

#endif /* _RATECONTROL_H_ */

