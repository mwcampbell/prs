#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mixerchannel.h"
#include "mp3decoder.h"
#include "mp3header.h"



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
	mp3_header_t mh;
	FILE *fp;
  
	fp = fopen (location, "rb");
	if (!fp)
		return NULL;
	mp3_header_read (fp, &mh);
	fclose (fp);
	/* Get sample information from file */

	ch = mixer_channel_new (mh.samplerate,
						   (mh.mode == 3) ? 1 : 2,
						   mixer_latency);

	d = mp3_decoder_new (location, 0);
	ch->data = (void *) d;
	ch->name = strdup (name);
	ch->location = strdup (location);

	/* Set overrideable methods */

	ch->get_data = mp3_mixer_channel_get_data;
	ch->free_data = mp3_mixer_channel_free_data;

	return ch;
}
