#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "mixerchannel.h"



MixerChannel *
mixer_channel_new (const int rate,
		   const int channels,
		   const int latency)
{
	MixerChannel *ch;

	ch = (MixerChannel *) malloc (sizeof(MixerChannel));

	ch->rate = rate;
	ch->channels = channels;
	ch->buffer_size = (int) ((double)latency/2*(double)rate/44100)*channels;
	ch->buffer = (short *) malloc (sizeof(short)*ch->buffer_size);

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
	
	return ch;
}
	

void
mixer_channel_destroy (MixerChannel *ch)
{
	if (!ch)
		return;
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
mixer_channel_get_data (MixerChannel *ch)
{
	int i;
	short *ptr;
	short *end_buffer;

	if (!ch)
		return;
	if (!ch->get_data)
		return;

	ch->get_data (ch);

	/* Fading and level processing */

	ptr = ch->buffer;
	end_buffer = ch->buffer+ch->buffer_length;
	while (ptr < end_buffer) {
		*ptr++ *= ch->level;
		if (ch->channels == 2)
			*ptr++ *= ch->level;
		if ((ch->fade < 0 && ch->level <= ch->fade_destination) ||
		    (ch->fade > 0 && ch->level >= ch->fade_destination))
			ch->fade = 0.0;
		if (ch->fade != 0.0)
			ch->level += ch->fade;
	}
}
