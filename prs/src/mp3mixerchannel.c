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
 * mp3mixerchannel.c: Implementation of a mixer channel to play mp3 files.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "mixerchannel.h"
#include "mp3decoder.h"
#include "mp3header.h"



static int
mp3_mixer_channel_get_data (MixerChannel *ch)
{
	MP3Decoder *d = NULL;
	int rv = -1;
  
	assert (ch != NULL);
	d = (MP3Decoder *) ch->data;
	assert (d != NULL);
	rv = mp3_decoder_get_data (d, ch->input, ch->chunk_size*ch->channels);

	return rv/ch->channels;
}



static void
mp3_mixer_channel_free_data (MixerChannel *ch)
{
	MP3Decoder *d = NULL;

	assert (ch != NULL);
	d = (MP3Decoder *) ch->data;
	assert (d != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "mp3_mixer_channel_free_data called for %s\n",
		      ch->name);
	mp3_decoder_destroy (d);
}



MixerChannel *
mp3_mixer_channel_new (const char *name, const char *location,
		       const int mixer_latency)
{
	MixerChannel *ch = NULL;
	MP3Decoder *d = NULL;
	mp3_header_t mh;
	FILE *fp;

	assert (name != NULL);
	assert (location != NULL);
	assert (mixer_latency > 0);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "mp3_mixer_channel_new (\"%s\", \"%s\", %d)\n",
		      name, location, mixer_latency);
	fp = fopen (location, "rb");
	if (!fp) {
		debug_printf (DEBUG_FLAGS_MIXER,
			      "mp3_mixer_channel_new: unable to open %s\n",
			      location);
		return NULL;
	}
	mp3_header_read (fp, &mh);
	fclose (fp);
	d = mp3_decoder_new (location, 0);
	if (d == NULL) {
		debug_printf (DEBUG_FLAGS_MIXER,
			      "mp3mixerchannel: failed to create decoder\n");
		return NULL;
	}
	ch = mixer_channel_new (mh.samplerate,
				(mh.mode == 3) ? 1 : 2,
				mixer_latency);
	ch->data = (void *) d;
	ch->name = strdup (name);
	ch->location = strdup (location);

	/* Set overrideable methods */

	ch->get_data = mp3_mixer_channel_get_data;
	ch->free_data = mp3_mixer_channel_free_data;

	mixer_channel_start_reader (ch);
	return ch;
}
