#ifndef _MULTIBAND_AUDIO_COMPRESSOR_H
#define _MULTIBAND_AUDIO_COMPRESSOR_H
#include "audiofilter.h"

AudioFilter *
multiband_audio_compressor_new (int rate,
		      int channels,
		      int buffer_size);
void
multiband_audio_compressor_add_band (AudioFilter *f,
			   double freq,
			   double threshhold,
			   double ratio,
			   double attack_time,
			   double release_time,
			   double volume,
				     double output_gain);


#endif
