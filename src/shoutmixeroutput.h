/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#ifndef _SHOUT_MIXER_OUTPUT_H
#define _SHOUT_MIXER_OUTPUT_H
#include <shout/shout.h>
#include "list.h"
#include "mixeroutput.h"



MixerOutput *
shout_mixer_output_new (const char *name,
			const int rate,
			const int channels,
			const int latency,
			shout_t *s,
			const int stereo,
			list *args_list,
			const char *archive_file_name,
			double retry_delay);
#endif
