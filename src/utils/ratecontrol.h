#ifndef _RATECONTROL_H_
#define _RATECONTROL_H_

#include "../portab.h"

typedef struct {
	int64_t size;
	int32_t count;
} QuantInfo;

typedef struct
{
	int32_t rtn_quant;
	int64_t frames;
	int64_t total_size;
	double framerate;
	int32_t target_rate;
	int16_t max_quant;
	int16_t min_quant;
	int64_t last_change;
	int64_t quant_sum;
	double quant_error[32];
	double avg_framesize;
	double target_framesize;
	double sequence_quality;
	int32_t averaging_period;
	int32_t reaction_delay_factor;
	int32_t buffer;
} RateControl;

void RateControlInit(RateControl *rate_control,
		     uint32_t target_rate,
		     uint32_t reaction_delay_factor,
		     uint32_t averaging_period,
		     uint32_t buffer, 
		     int framerate,
		     int max_quant,
		     int min_quant);

int RateControlGetQ(RateControl *rate_control,
		    int keyframe);

void RateControlUpdate(RateControl *rate_control,
		       int16_t quant,
		       int frame_size,
		       int keyframe);

#endif /* _RATECONTROL_H_ */

