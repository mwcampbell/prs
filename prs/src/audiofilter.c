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
 * audiofilter.c: Implements the base class for audio filters.
 *
 */

#include <assert.h>
#include <stdlib.h>
#include "debug.h"
#include "audiofilter.h"



AudioFilter *
audio_filter_new (int rate,
		  int channels,
		  int latency)
{
	AudioFilter *f = NULL;

	debug_printf (DEBUG_FLAGS_MIXER,
		      "audio_filter_new (%d, %d, %d)\n",
		      rate, channels, latency);
	assert (rate > 0);
	assert (channels > 0);
	assert (latency > 0);
	f = (AudioFilter *) malloc (sizeof (AudioFilter));
	assert (f != NULL);
	f->rate = rate;
	f->channels = channels;
	f->buffer_size = latency/44100.0*rate;
	debug_printf (DEBUG_FLAGS_MIXER,
		      "audio_filter_new: buffer_size = %d\n",
		      f->buffer_size);
	f->buffer = (short *) malloc (f->buffer_size*sizeof(short)*channels);
	assert (f->buffer != NULL);
	f->buffer_length = 0;
	f->data = NULL;
	f->process_data = NULL;
	return f;
}




int
audio_filter_process_data (AudioFilter *f,
			   short *input,
			   int input_length,
			   short *output,
			   int output_length)
{
	assert (f != NULL);
	assert (f->process_data != NULL);
	assert (input != NULL);
	assert (input_length >= 0);
	assert (output != NULL);
	assert (output_length >= 0);
	return f->process_data (f,
				input,
				input_length*f->channels,
				output,
				output_length*f->channels);
}
