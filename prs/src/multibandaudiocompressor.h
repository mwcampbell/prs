#ifndef _AUDIO_COMPRESSOR_H
#define _AUDIO_COMPRESSOR_H
#include "audiofilter.h"

AudioFilter *
audio_compressor_new (int rate,
		      int channels,
		      int buffer_size);
void
audio_compressor_add_band (AudioFilter *f,
			   double freq,
			   double threshhold,
			   double ratio,
			   double attack_time,
			   double release_time,
			   double output_gain);


#endif
