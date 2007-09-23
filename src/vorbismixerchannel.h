/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#ifndef _VORBIS_MIXER_CHANNEL_H
#define _VORBIS_MIXER_CHANNEL_H
#include "mixer.h"



MixerChannel *
vorbis_mixer_channel_new (const char *name,
			  const char *location,
			  const int mixer_latency);

#endif
