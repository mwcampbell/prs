#ifndef _AUDIO_COMPRESSOR_H
#define _AUDIO_COMPRESSOR_H
#include "audiofilter.h"

AudioFilter *
audio_compressor_new (int rate,
		      int channels,
		      int buffer_size,
		      double threshhold,
		      double ratio,
		      double attack_time,
		      double release_time,
		      double output_gain);


#endif
