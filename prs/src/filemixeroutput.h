#ifndef _FILE_MIXER_CHANNEL_H
#define _FILE_MMIXER_CHANNEL_H
#include "mixer.h"



MixerChannel *
file_mixer_channel_new (const char *name,
		       int rate,
		       int channels);

#endif
