#ifndef _OSS_MIXER_CHANNEL_H
#define _OSS_MMIXER_CHANNEL_H
#include "mixer.h"



MixerChannel *
oss_mixer_channel_new (const char *name,
		       int rate,
		       int channels,
		       int fd);

#endif
