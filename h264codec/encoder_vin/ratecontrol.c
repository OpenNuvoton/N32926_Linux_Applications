#include "ratecontrol.h"

void H264RateControlInit(H264RateControl * rate_control,
				unsigned int target_rate,
				unsigned int reaction_delay_factor,
				unsigned int averaging_period,
				unsigned int buffer,
				float framerate,
				int max_quant,
				int min_quant,
				unsigned int initq,
				unsigned int IPInterval)
{
	int i;
  /*
	if(buffer <= 50){
		rate_control->frames = 0;
		rate_control->total_size = 0;
	}else{
		rate_control->frames = framerate * 1;
		rate_control->total_size = target_rate * 1 / 2;
	}
  */
	rate_control->frames = 0;
	rate_control->total_size = 0;

	//rate_control->framerate = (double) framerate / 1000.0;
	rate_control->framerate = (double) framerate;
	rate_control->IPInterval= IPInterval;
	rate_control->IPIntervalCnt= 0;
	rate_control->target_rate = (int) target_rate;
//	rate_control->rtn_quant = get_initial_quant(target_rate);
	rate_control->rtn_quant = initq;
	rate_control->pre_rtn_quant = initq;
	rate_control->max_quant = (short) max_quant;
	rate_control->min_quant = (short) min_quant;
	for (i = 0; i < rc_MAX_QUANT; ++i) {
		rate_control->quant_error[i] = 0.0;
	}
	rate_control->target_framesize =
		(double) target_rate / 8.0 / rate_control->framerate;
	rate_control->sequence_quality = 2.0 / (double) rate_control->rtn_quant;
	rate_control->avg_framesize = rate_control->target_framesize;
	rate_control->reaction_delay_factor = (int) reaction_delay_factor;
	rate_control->averaging_period = (int) averaging_period;
	rate_control->buffer = (int) buffer;
}


void H264RateControlUpdate(H264RateControl *rate_control,
				  short quant,
				  int frame_size,
				  int keyframe)
{
	//long long deviation;
	int64_t deviation;
	double overflow, averaging_period, reaction_delay_factor;
	double quality_scale, base_quality, target_quality;
	int rtn_quant;

	rate_control->rtn_quant = rate_control->pre_rtn_quant;
	if ((quant == rate_control->min_quant) && (frame_size < rate_control->target_framesize))
		goto skip_integrate_err;
	else if ((quant == rate_control->max_quant) && (frame_size > rate_control->target_framesize))
		goto skip_integrate_err;

	rate_control->frames++;
	rate_control->total_size += frame_size;

skip_integrate_err:
	deviation =
		/*(long long)*/(int64_t) ((double) rate_control->total_size -
				   ((double) ((double) rate_control->target_rate / 8.0 /
					 (double) rate_control->framerate) * (double) rate_control->frames));

	if (rate_control->rtn_quant >= 2) {
		averaging_period = (double) rate_control->averaging_period;
		rate_control->sequence_quality -=
			rate_control->sequence_quality / averaging_period;
		rate_control->sequence_quality +=
			2.0 / (double) rate_control->rtn_quant / averaging_period;
		if (rate_control->sequence_quality < 0.1)
			rate_control->sequence_quality = 0.1;
		if (!keyframe) {
			reaction_delay_factor =
				(double) rate_control->reaction_delay_factor;
			rate_control->avg_framesize -=
				rate_control->avg_framesize / reaction_delay_factor;
			rate_control->avg_framesize += frame_size / reaction_delay_factor;
		}
	}

	quality_scale =
		rate_control->target_framesize / rate_control->avg_framesize *
		rate_control->target_framesize / rate_control->avg_framesize;

	base_quality = rate_control->sequence_quality;
	if (quality_scale >= 1.0) {
		base_quality = 1.0 - (1.0 - base_quality) / quality_scale;
	} else {
		//base_quality = 0.06452 + (base_quality - 0.06452) * quality_scale;
		base_quality = quality_const + (base_quality - quality_const) * quality_scale;
	}
	overflow = -((double) deviation / (double) rate_control->buffer);
  /*
	target_quality =
		base_quality + (base_quality -
						0.06452) * overflow / rate_control->target_framesize;*/
	target_quality =
		base_quality + (base_quality - quality_const) * overflow / rate_control->target_framesize;
	/*
	if (target_quality > 2.0)
		target_quality = 2.0;
	else if (target_quality < 0.06452)
		target_quality = 0.06452;*/
	if (target_quality > 2.0)
		target_quality = 2.0;
	else if (target_quality < quality_const)
		target_quality = quality_const;

	rtn_quant = (int) (2.0 / target_quality);
	if (rtn_quant < rc_MAX_QUANT) {
		rate_control->quant_error[rtn_quant] +=
			2.0 / target_quality - rtn_quant;
		if (rate_control->quant_error[rtn_quant] >= 1.0) {
			rate_control->quant_error[rtn_quant] -= 1.0;
			rtn_quant++;
		}
	}
	if(rate_control->framerate<=10) {
		if (rtn_quant > rate_control->rtn_quant + 3)
			rtn_quant = rate_control->rtn_quant + 3;
		else if (rtn_quant < rate_control->rtn_quant - 3)
			rtn_quant = rate_control->rtn_quant - 3;
	}else{
		if (rtn_quant > rate_control->rtn_quant + 1)
			rtn_quant = rate_control->rtn_quant + 1;
		else if (rtn_quant < rate_control->rtn_quant - 1)
			rtn_quant = rate_control->rtn_quant - 1;
	}
		 
	if (rtn_quant > rate_control->max_quant)
		rtn_quant = rate_control->max_quant;
	else if (rtn_quant < rate_control->min_quant)
		rtn_quant = rate_control->min_quant;
	rate_control->pre_rtn_quant = rtn_quant;
	if(rate_control->framerate <= 10 && rate_control->target_rate <= 128000) {
		if(++rate_control->IPIntervalCnt % rate_control->IPInterval == 0) {
			rtn_quant -= 5;
			if(rtn_quant <= 25){
				rtn_quant = 25;
			}
			//rate_control->IPIntervalCnt =0;
		}
	}
	if(keyframe == 1)
		rate_control->IPIntervalCnt=1; 
	rate_control->rtn_quant = rtn_quant;
}
