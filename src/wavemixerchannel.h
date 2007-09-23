/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#ifndef _WAVE_MIXER_CHANNEL_H
#define _WAVE_MIXER_CHANNEL_H
#include "mixer.h"



MixerChannel *
wave_mixer_channel_new (const char *name,
			  const char *location,
			  const int mixer_latency);

#endif
