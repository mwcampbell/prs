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
 * mixeroutput.h: header file for the base class for mixer outputs.
 *
 */

#ifndef _MIXER_OUTPUT_H
#define _MIXER_OUTPUT_H

#include "list.h"
#include "audiofilter.h"






typedef struct _MixerOutput MixerOutput;



struct _MixerOutput {
  char *name;
  
  int enabled;

  /* Sample information */

  int rate;
  int channels;

  /* output buffer */

  short *buffer;
  int buffer_size;

  /* user data */

  void *data;

  /* Overrideable methods */

  void (*free_data) (MixerOutput *o);
  void (*post_data) (MixerOutput *o);
};



void
mixer_output_destroy (MixerOutput *o);
void
mixer_output_alloc_buffer (MixerOutput *o,
			   const int latency);
int
mixer_output_add_data (MixerOutput *o,
			 short *buffer,
			 int length);
void
mixer_output_post_data (MixerOutput *o);
void
mixer_output_reset_data (MixerOutput *o);
#endif
