#ifndef _MIXER_H
#define _MIXER_H
#include <pthread.h>
#include "list.h"
#include "mixerchannel.h"
#include "mixerbus.h"
#include "mixeroutput.h"


typedef struct _mixer mixer;

struct _mixer {
  pthread_t thread;
  pthread_mutex_t mutex;


  /* Notification */

  pthread_cond_t notify_condition;
  double notify_time;
  
  /* Flag indicating whether mixer is running */

  int running;
  
  double cur_time;

  list *channels;
  list *busses;
  list *outputs;
};



mixer *
mixer_new (void);
int
mixer_start (mixer *m);
int
mixer_stop (mixer *m);
void
mixer_destroy (mixer *m);
void
mixer_add_channel (mixer *m,
		   MixerChannel *ch);
void
mixer_delete_channel (mixer *m,
		      const char *channel_name);
void
mixer_delete_all_channels (mixer *m);
MixerChannel *
mixer_get_channel (mixer *m,
		   const char *channel_name);
void
mixer_add_bus (mixer *m,
		  MixerBus *b);
void
mixer_delete_bus (mixer *m,
		     const char *bus_name);
MixerBus *
mixer_get_bus (mixer *m,
	       const char *bus_name);
void
mixer_add_output (mixer *m,
		  MixerOutput *o);
void
mixer_delete_output (mixer *m,
		     const char *output_name);
MixerOutput *
mixer_get_output (mixer *m,
		  const char *output_name);
void
mixer_patch_channel (mixer *m,
		     const char *channel_name,
		     const char *bus_name);
void
mixer_patch_channel_all (mixer *m,
			 const char *channel_name);
void
mixer_patch_bus (mixer *m,
		 const char *bus_name,
		 const char *output_name);
double
mixer_get_time (mixer *m);
void
mixer_sync_time (mixer *m);
void
mixer_fade_channel (mixer *m,
		    const char *channel_name,
		    double fade_destination,
		    double fade_time);
void
mixer_fade_all (mixer *m,
		double level,
		double fade_time);
void
mixer_reset_notification_time (mixer *m,
			     double notification_time);
void
mixer_wait_for_notification (mixer *m,
			     double notify_time);



#endif
