#ifndef _MIXER_CHANNEL_H_
#define _MIXER_CHANNEL_H_
#include <pthread.h>
#include "list.h"


typedef struct _MixerChannel MixerChannel;


struct  _MixerChannel {
	char *name;
	char *location;
	int enabled;
  
	/* Sample information */

	int rate;
	int channels;

	/* buffer */
	
	pthread_mutex_t mutex;
	pthread_t data_reader_thread;
	short *buffer;
	short *input;
	short *output;
	short *buffer_end;
	int chunk_size;
	int this_chunk_size;
	int buffer_size;
	int space_left;
	
        /* Level and fading parameters */

	double level;
	double fade;
	double fade_destination;

	/* end of data indicator */

	int data_end_reached;

	void *data;

	/* List of patchpoints */

	list *patchpoints;

	/* overrideable methods */

	void (*free_data) (MixerChannel *ch);
	int (*get_data) (MixerChannel *ch);
};



MixerChannel *
mixer_channel_new (const int rate,
		   const int channels,
		   const int latency);
void
mixer_channel_destroy (MixerChannel *ch);
int
mixer_channel_get_data (MixerChannel *ch);
void
mixer_channel_start_reader (MixerChannel *ch);
void
mixer_channel_process_levels (MixerChannel *ch);
void
mixer_channel_advance_pointers (MixerChannel *ch);


#endif
