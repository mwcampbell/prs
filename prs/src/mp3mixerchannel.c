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
