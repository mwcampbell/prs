#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "debug.h"
#include "mixerchannel.h"



static void *
data_reader (void *data)
{
	int done = 0;
	MixerChannel *ch = (MixerChannel *) data;

	while (!done) {
		if (ch->space_left && ch->get_data && ch->enabled) {
			int rv = mixer_channel_get_data (ch);
			if (rv < ch->chunk_size)
				done = 1;
		}
		else {
			usleep ((double)(ch->chunk_size/ch->rate)*1000000);
		}
	}
	ch->data_reader_thread = -1;
}


MixerChannel *
mixer_channel_new (const int rate,
		   const int channels,
		   const int latency)
{
	MixerChannel *ch = NULL;

	debug_printf (DEBUG_FLAGS_MIXER,
		      "mixer_channel_new (%d, %d, %d)\n",
		      rate, channels, latency);
	assert (rate > 0);
	assert (channels > 0);
	assert (latency > 0);
	ch = (MixerChannel *) malloc (sizeof (MixerChannel));
	assert (ch != NULL);
	ch->rate = rate;
	ch->channels = channels;
	ch->chunk_size = (latency/44100.0)*rate;
	ch->space_left = ch->buffer_size = ch->chunk_size*100;
	debug_printf (DEBUG_FLAGS_MIXER,
		      "mixer_channel_new: buffer_size = %d\n",
		      ch->buffer_size);
	ch->input = ch->output = ch->buffer =
		(short *) malloc (sizeof(short)*ch->buffer_size*channels);
	assert (ch->buffer != NULL);
	ch->buffer_end = ch->buffer+ch->buffer_size*ch->channels;

	/* Defaults to patched to no busses */

	ch->patchpoints = NULL;
	ch->enabled = 0;
	ch->data_end_reached = 0;
	ch->level = 1.0;
	ch->fade = 0.0;
	ch->fade_destination = 1.0;

	/* Overrideable functions */

	ch->free_data = NULL;
	ch->get_data = NULL;
	
	/* Sart the data reader */

	pthread_mutex_init (&(ch->mutex), NULL);
	ch->data_reader_thread = 0;
	return ch;
}
	

void
mixer_channel_destroy (MixerChannel *ch)
{
	assert (ch != NULL);
	debug_printf (DEBUG_FLAGS_MIXER, "mixer_channel_destroy called\n");
	if (ch->name)
		free (ch->name);
	if (ch->location)
		free (ch->location);
	if (ch->free_data)
		ch->free_data (ch);
	else {
		if (ch->data)
			free (ch->data);
	}

	/* Free the buffer */

	if (ch->buffer)
		free (ch->buffer);

        /* We don't own the patch points, so just free the list */

	list_free (ch->patchpoints);
	free (ch);
}



void
mixer_channel_start_reader (MixerChannel *ch)
{
	if (ch->data_reader_thread == 0)
		pthread_create (&(ch->data_reader_thread), NULL, data_reader, ch);
}


int
mixer_channel_get_data (MixerChannel *ch)
{
	int rv;

	rv = ch->get_data (ch);
	if (rv < ch->chunk_size && ch->data_reader_thread == 0)
		ch->data_end_reached = 1;
	ch->input += rv*ch->channels;
	if (ch->input >= ch->buffer_end)
		ch->input = ch->buffer;
	ch->space_left -= rv;
	return rv;
}
