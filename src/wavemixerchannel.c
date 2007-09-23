/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "mixerchannel.h"
#include "wave.h"



static int
wave_mixer_channel_get_data (MixerChannel *ch)
{
	int rv = -1;
	FILE *fp;

	assert (ch != NULL);
	fp = (FILE *) ch->data;
	
	rv = fread (ch->input, sizeof(short), ch->chunk_size*ch->channels, fp);
	return rv/ch->channels;
}



static void
wave_mixer_channel_free_data (MixerChannel *ch)
{
	FILE *fp;

	assert (ch != NULL);
	fp = (FILE *) ch->data;
	fclose (fp);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "wave_mixer_channel_free_data called for %s\n",
		      ch->name);
}



MixerChannel *
wave_mixer_channel_new (const char *name, const char *location,
		       const int mixer_latency)
{
	MixerChannel *ch = NULL;
	FILE *fp;
	riff_header riff;
	chunk_header format_chunk_header;
	format_header format_chunk;
	chunk_header data_chunk_header;

	assert (name != NULL);
	assert (location != NULL);
	assert (mixer_latency > 0);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "wave_mixer_channel_new (\"%s\", \"%s\", %d)\n",
		      name, location, mixer_latency);
	fp = fopen (location, "rb");
	if (!fp) {
		debug_printf (DEBUG_FLAGS_MIXER,
			      "wave_mixer_channel_new: unable to open %s\n",
			      location);
		return NULL;
	}
	/* Read the headers */

	fread (&riff, sizeof(riff_header), 1, fp);
	fread (&format_chunk_header, sizeof(chunk_header), 1, fp);
	fread (&format_chunk, sizeof(format_header), 1, fp);
	fread (&data_chunk_header, sizeof(chunk_header), 1, fp);

	ch = mixer_channel_new (format_chunk.samples_per_second,
				format_chunk.channels,
				mixer_latency);

	ch->data = (void *) fp;
	ch->name = strdup (name);
	ch->location = strdup (location);
	ch->rate = format_chunk.samples_per_second;
	ch->channels = format_chunk.channels;

	/* Set overrideable methods */

	ch->get_data = wave_mixer_channel_get_data;
	ch->free_data = wave_mixer_channel_free_data;

	mixer_channel_start_reader (ch);
	return ch;
}
