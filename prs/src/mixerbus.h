/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
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
