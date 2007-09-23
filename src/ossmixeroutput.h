/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#ifndef _OSS_MIXER_OUTPUT_H
#define _OSS_MIXER_OUTPUT_H
#include "mixeroutput.h"



MixerOutput *
oss_mixer_output_new (const char *name,
		      int rate,
		      int channels,
		      int latency);
#endif
