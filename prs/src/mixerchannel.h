#ifndef _MIXER_CHANNEL_H_
#define _MIXER_CHANNEL_H_
#include "list.h"


typedef struct _MixerChannel MixerChannel;



struct _MixerChannel {
  char *name;
  char *location;
  int enabled;
  
  /* Sample information */

  int rate;
  int channels;

  /* Level and fading parameters */

  double level;
  double fade;
  double fade_destination;

  /* end of data indicator */

  int data_end_reached;

  void *data;

  /* List of busses to which this channel is patched */

  list *busses;

  /* overrideable methods */

  void (*free_data) (MixerChannel *ch);
  int (*get_data) (MixerChannel *ch, short *buffer, int size);
};



void
mixer_channel_destroy (MixerChannel *ch);
int
mixer_channel_get_data (MixerChannel *ch,
			short *buffer,
			int size);

#endif
