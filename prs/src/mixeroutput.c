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
 * mixeroutput.c: Implementation of the base class for mixer outputs.
 *
 */

#include <assert.h>
#include "debug.h"
#include "mixeroutput.h"



void
mixer_output_destroy (MixerOutput *o)
{
	assert (o != NULL);
	debug_printf (DEBUG_FLAGS_MIXER, "mixer_output_destroy called\n");
	if (o->free_data)
		o->free_data (o);
	else {
		if (o->data)
			free (o->data);
	}
	if (o->name)
		free (o->name);
	if (o->buffer)
		free (o->buffer);
	free (o);
}



void
mixer_output_alloc_buffer (MixerOutput *o,
			   const int latency)
{
	assert (o != NULL);
	assert (latency > 0);
	o->buffer_size = latency/44100.0*o->rate;
	debug_printf (DEBUG_FLAGS_MIXER,
		      "mixer_output_alloc_buffer: buffer_size = %d\n",
		      o->buffer_size);
	o->buffer = (short *) malloc (o->buffer_size*sizeof(short)*o->channels);
	assert (o->buffer != NULL);
}



int
mixer_output_add_data (MixerOutput *o,
		       short *buffer,
		       int length)
{
	short *tmp, *tmp2;
  
	assert (o != NULL);
	assert (o->buffer != NULL);
	assert (buffer != NULL);
	assert (length >= 0);
	tmp = o->buffer;
	tmp2 = buffer;

	length *= o->channels;
	while (length--)
		*tmp++ += *tmp2++;
}



void
mixer_output_post_data (MixerOutput *o)
{
	assert (o != NULL);
	assert (o->post_data != NULL);
	o->post_data (o);
}



void
mixer_output_reset_data (MixerOutput *o)
{
	assert (o != NULL);
	assert (o->buffer != NULL);
	memset (o->buffer, 0, (o->buffer_size)*sizeof(short)*o->channels);
}
