/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * shoutmixeroutput.h: Mixer output which streams to a shoutcast/icecast
 * server.
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
			const char *archive_file_name);
#endif
