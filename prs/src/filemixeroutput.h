#ifndef _FILE_MIXER_OUTPUT_H
#define _FILE_MIXER_OUTPUT_H
#include "mixer.h"



MixerOutput *
file_mixer_output_new (const char *name,
		       int rate,
		       int channels,
		       int latency);

#endif
