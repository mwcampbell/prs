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

	/* buffer */
	
	short *buffer;
	int buffer_size;
	int buffer_length;
	
        /* Level and fading parameters */

	double level;
	double fade;
	double fade_destination;

	/* end of data indicator */

	int data_end_reached;

	void *data;

	/* List of busses to which this channel is patched */

	list *patchpoints;

	/* overrideable methods */

	void (*free_data) (MixerChannel *ch);
	void (*get_data) (MixerChannel *ch);
};



MixerChannel *
mixer_channel_new (int rate,
		   int channels,
		   int latency);
void
mixer_channel_destroy (MixerChannel *ch);
void
mixer_channel_get_data (MixerChannel *ch);

#endif
