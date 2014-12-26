/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#ifndef _ALSA_MIXER_OUTPUT_H
#define _ALSA_MIXER_OUTPUT_H
#include "mixeroutput.h"



MixerOutput *
alsa_mixer_output_new (const char *sc_name,
		       const char *name,
		       int rate,
		       int channels,
		       int latency);
#endif
