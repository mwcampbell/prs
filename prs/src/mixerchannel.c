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
 * mixerchannel.c: Implementation of the base class for mixer channels.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include "debug.h"
#include "mixerchannel.h"



static void *
data_reader (void *data)
{
	int done = 0;
	MixerChannel *ch = (MixerChannel *) data;

	while (!done) {

		/* Copy chnanel variables into our local space */

		int space_left;
		int data_end_reached;
		int chunk_size;
		void *get_data;
		
		pthread_mutex_lock (&(ch->mutex));
		space_left = ch->space_left;
		data_end_reached = ch->data_end_reached;
		chunk_size = ch->chunk_size;
		get_data = ch->get_data;
		pthread_mutex_unlock (&(ch->mutex));
		
		if (!data_end_reached && space_left && ch->get_data) {
			int rv = mixer_channel_get_data (ch);
			if (rv < chunk_size)
				done = 1;
		}
		else if (!data_end_reached)
			usleep ((double)(ch->chunk_size/ch->rate)*1000000);
		else
			done = 1;
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
	ch->this_chunk_size = 0;
	ch->space_left = ch->buffer_size = ch->chunk_size*50;
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
	ch->key = -1;
	
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


	/* Ensure the data reader quits */

	if (ch->data_reader_thread > 0) {
		ch->data_end_reached = 1;
		pthread_join ((&ch->data_reader_thread), NULL);
	}

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

	pthread_mutex_lock (&(ch->mutex));
	rv = ch->get_data (ch);
	ch->input += rv*ch->channels;
	if (ch->input >= ch->buffer_end)
		ch->input = ch->buffer;
	ch->space_left -= rv;
	pthread_mutex_unlock (&(ch->mutex));
	return rv;
}


void
mixer_channel_process_levels (MixerChannel *ch)
{
	short *tmp;
	int j;
	
	/* Compute number of samples in this chunk */

	if (ch->input > ch->output)
		ch->this_chunk_size = ch->input-ch->output;
	else if (ch->space_left != ch->buffer_size)
		ch->this_chunk_size = ch->buffer_end-ch->output;
	else
		ch->this_chunk_size = 0;
	ch->this_chunk_size  /= ch->channels;
	if (ch->this_chunk_size >= ch->chunk_size)
		ch->this_chunk_size = ch->chunk_size;
	if (ch->level != 1.0 || ch->fade != 1.0) {
		if (ch->level != 1.0 || ch->fade != 1.0) {
			tmp = ch->output;
			j = ch->this_chunk_size;
			while (j--) {
				*tmp++ *= ch->level;
				if (ch->channels == 2) {
					*tmp++ *= ch->level;
				}
				if ((ch->fade < 1.0 && ch->level <= ch->fade_destination) ||
				    (ch->fade > 1.0 && ch->level >= ch->fade_destination)) {
					ch->level = ch->fade_destination;
					ch->fade = 1.0;
				}
				if (ch->fade != 1.0)
					ch->level *= ch->fade;
			}
		}
	}
}


void
mixer_channel_advance_pointers (MixerChannel *ch)
{
	pthread_mutex_lock (&(ch->mutex));
	ch->output += ch->this_chunk_size*ch->channels;
	if (ch->output >= ch->buffer_end)
		ch->output = ch->buffer;
	ch->space_left += ch->this_chunk_size;
	pthread_mutex_unlock (&(ch->mutex));
}
