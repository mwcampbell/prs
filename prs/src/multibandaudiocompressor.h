/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#ifndef _MULTIBAND_AUDIO_COMPRESSOR_H
#define _MULTIBAND_AUDIO_COMPRESSOR_H
#include "audiofilter.h"

AudioFilter *
multiband_audio_compressor_new (int rate,
		      int channels,
		      int buffer_size);
void
multiband_audio_compressor_add_band (AudioFilter *f,
				     float freq,
				     float threshhold,
				     float ratio,
				     float attack_time,
				     float release_time,
				     float volume,
				     float output_gain,
				     float link);


#endif
