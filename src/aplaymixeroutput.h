/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#ifndef _APLAY_MIXER_OUTPUT_H
#define _APLAY_MIXER_OUTPUT_H
#include "mixeroutput.h"



MixerOutput *
aplay_mixer_output_new (const char *name,
		        int rate,
		        int channels,
		        int latency);
#endif
