/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#ifndef _AUDIO_COMPRESSOR_H
#define _AUDIO_COMPRESSOR_H
#include "audiofilter.h"

AudioFilter *
audio_compressor_new (int rate,
		      int channels,
		      int buffer_size,
		      float threshhold,
		      float ratio,
		      float attack_time,
		      float release_time,
		      float output_gain);


#endif
