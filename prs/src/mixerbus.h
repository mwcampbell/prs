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
 * mixerbus.h: header file for the mixer bus object.
 *
 */

#ifndef _MIXER_BUS_H
#define _MIXER_BUS_H

#include "list.h"
#include "mixeroutput.h"
#include "audiofilter.h"



typedef struct _MixerBus MixerBus;



struct _MixerBus {
  char *name;
  
  int enabled;

  /* Sample information */

  int rate;
  int channels;

  /* buffer */

  short *buffer;
  int buffer_size;
  short *process_buffer;
  int process_buffer_size;
  
  /* List of outputs to which this bus is patched */

  list *outputs;

  /* Audio filters */

  list *filters;
};



MixerBus *
mixer_bus_new (const char *name,
	       int rate,
	       int channels,
	       int latency);
void
mixer_bus_destroy (MixerBus *b);
int
mixer_bus_add_data (MixerBus *b,
			 short *buffer,
			 int length);
void
mixer_bus_post_data (MixerBus *b);
void
mixer_bus_reset_data (MixerBus *b);
void
mixer_bus_add_output (MixerBus *b,
		      MixerOutput *o);
void
mixer_bus_add_filter (MixerBus *b,
		      AudioFilter *f);
#endif
