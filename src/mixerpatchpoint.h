/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#ifndef _MIXEr_PATCH_POINT_h
#define _MIXER_PATCH_POINT_H
#include "resample.h"
#include "mixerchannel.h"
#include "mixerbus.h"



typedef struct {
	int no_processing;
	res_state *resampler;
	MixerChannel *ch;
	MixerBus *bus;

	/* Buffers */

	short *tmp_buffer;
	int tmp_buffer_size;
	int tmp_buffer_length;
	SAMPLE *input_buffer;
	int input_buffer_size;
	int input_buffer_length;
	SAMPLE *output_buffer;
	int output_buffer_size;
	int output_buffer_length;
} MixerPatchPoint;



MixerPatchPoint *
mixer_patch_point_new (MixerChannel *ch,
		       MixerBus *b,
		       int latency);
void
mixer_patch_point_post_data (MixerPatchPoint *p);
void
mixer_patch_point_destroy (MixerPatchPoint *p);



#endif
