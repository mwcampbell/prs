#ifndef _SHOUT_MIXER_OUTPUT_H
#define _SHOUT_MIXER_OUTPUT_H
#include "mixeroutput.h"



MixerOutput *
shout_mixer_output_new (const char *name,
		      int rate,
		      int channels,
			const char * ip,
			int port,
			const char *password);
#endif
