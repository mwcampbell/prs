#ifndef _SHOUT_MIXER_OUTPUT_H
#define _SHOUT_MIXER_OUTPUT_H
#include <shout/shout.h>
#include "mixeroutput.h"



MixerOutput *
shout_mixer_output_new (const char *name,
			int rate,
			int channels,
			int latency,
			shout_t *s,
			int stereo);
#endif
