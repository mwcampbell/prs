#ifndef _OSS_MIXER_OUTPUT_H
#define _OSS_MIXER_OUTPUT_H
#include "mixeroutput.h"



MixerOutput *
oss_mixer_output_new (const char *name,
		      int rate,
		      int channels);
#endif
