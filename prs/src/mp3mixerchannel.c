#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mixerchannel.h"
#include "mp3decoder.h"
#include "mp3tech.h"



static void
mp3_mixer_channel_get_data (MixerChannel *ch)
{
	MP3Decoder *d = NULL;
	int rv = -1;
  
	if (!ch)
		return;
	d = (MP3Decoder *) ch->data;
	rv = mp3_decoder_get_data (d, ch->buffer, ch->buffer_size);

	if (rv <= 0)
		ch->data_end_reached = 1;
	ch->buffer_length = rv;
	
	return;
}



static void
mp3_mixer_channel_free_data (MixerChannel *ch)
{
	MP3Decoder *d = NULL;

	if (!ch)
		return;
	d = (MP3Decoder *) ch->data;
	mp3_decoder_destroy (d);
}



MixerChannel *
mp3_mixer_channel_new (const char *name, const char *location,
		       const int mixer_latency)
{
	MixerChannel *ch = NULL;
	MP3Decoder *d = NULL;
	mp3info mp3;
  
	memset (&mp3, 0, sizeof (mp3));
	mp3.file = fopen (location, "rb");
	if (!mp3.file)
		return NULL;
	mp3.filename = strdup (location);
	get_mp3_info (&mp3, SCAN_QUICK, 0);
	fclose (mp3.file);
	mp3.file = NULL;
	free (mp3.filename);
	mp3.filename = NULL;
	/* Get sample information from file */

	ch = mixer_channel_new (header_frequency (&mp3.header),
						   (mp3.header.mode == 3) ? 1 : 2,
						   mixer_latency);

	d = mp3_decoder_new (location, 0);
	ch->data = (void *) d;
	ch->name = strdup (name);
	ch->location = strdup (location);
	ch->enabled = 1;
  
	/* Set overrideable methods */

	ch->get_data = mp3_mixer_channel_get_data;
	ch->free_data = mp3_mixer_channel_free_data;

	/* Set level and fading parameters */

	ch->fade = 0.0;
	ch->level = 1.0;
	ch->fade_destination = 1.0;

	/* Set the end of data flag to 0 */

	ch->data_end_reached = 0;

	return ch;
}
