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
 * audiofilter.h: header for the base class for audio filters.
 *
 */

#ifndef _AUDIO_FILTER_H
#define _AUDIO_FILTER_H

#include <stdio.h>



typedef struct _AudioFilter AudioFilter;

struct _AudioFilter {
  int rate;
  int channels;
  short *buffer;
  int buffer_size;
  int buffer_length;
  void *data;

  /* Functions */

  int (*process_data) (AudioFilter *filter,
		       short *input_buffer,
		       int input_size,
		       short *output_buffer,
		       int output_size);
};




AudioFilter *
audio_filter_new (int rate,
		  int channels,
		  int latency);
int
audio_filter_process_data (AudioFilter *f,
			   short *input,
			   int input_length,
			   short *output,
			   int output_length);



#endif
