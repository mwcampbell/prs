/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#ifndef _OSS_MIXER_CHANNEL_H
#define _OSS_MMIXER_CHANNEL_H
#include "mixer.h"



MixerChannel *
oss_mixer_channel_new (const char *name,
		       int rate,
		       int channels,
		       const int latency);

#endif
