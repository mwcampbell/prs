#ifndef _MIXER_CHANNEL_H_
#define _MIXER_CHANNEL_H_
#include "list.h"


typedef struct _MixerChannel MixerChannel;



struct _MixerChannel {
  char *name;
  char *location;
  
  /* Sample information */

  int rate;
  int channels;

  void *data;

  /* List of outputs to which this channel is patched */

  list *outputs;

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
