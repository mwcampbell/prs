/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
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
