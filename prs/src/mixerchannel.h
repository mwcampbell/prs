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
 * mixerchannel.h: header file for the base class for mixer channels.
 *
 */

#ifndef _MIXER_CHANNEL_H_
#define _MIXER_CHANNEL_H_
#include <pthread.h>
#include "list.h"


typedef struct _MixerChannel MixerChannel;


struct  _MixerChannel {
	int key;
	char *name;
	char *location;
	int enabled;
  
	/* Sample information */

	int rate;
	int channels;

	/* buffer */
	
	pthread_mutex_t mutex;

	/* Reader thread stuff */

	int reader_thread_running;
	pthread_t data_reader_thread;

	short *buffer;
	short *input;
	short *output;
	short *buffer_end;
	int chunk_size;
	int this_chunk_size;
	int buffer_size;
	int space_left;
	
        /* Level and fading parameters */

	double level;
	double fade;
	double fade_destination;

	/* end of data indicator */

	int data_end_reached;

	void *data;

	/* List of patchpoints */

	list *patchpoints;

	/* overrideable methods */

	void (*free_data) (MixerChannel *ch);
	int (*get_data) (MixerChannel *ch);
};



MixerChannel *
mixer_channel_new (const int rate,
		   const int channels,
		   const int latency);
void
mixer_channel_destroy (MixerChannel *ch);
int
mixer_channel_get_data (MixerChannel *ch);
void
mixer_channel_start_reader (MixerChannel *ch);
void
mixer_channel_process_levels (MixerChannel *ch);
void
mixer_channel_advance_pointers (MixerChannel *ch);


#endif
