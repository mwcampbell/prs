#ifndef _URL_MIXER_CHANNEL_H
#define _URL_MIXER_CHANNEL_H
#include "mixer.h"



MixerChannel *
url_mixer_channel_new (const char *name,
			  const char *location,
		       const char *archive_file_name,
		       const int mixer_latency);

#endif
