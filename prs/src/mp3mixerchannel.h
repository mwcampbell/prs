/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#ifndef _MP3_MIXER_CHANNEL_H
#define _MP3_MIXER_CHANNEL_H
#include "mixer.h"



MixerChannel *
mp3_mixer_channel_new (const char *name, const char *location,
		       const int mixer_latency);

#endif
