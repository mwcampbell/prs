#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <malloc.h>
#include "mixer.h"
#include "mixerchannel.h"
#include "mixerbus.h"
#include "mixerpatchpoint.h"
#include "mixeroutput.h"



static void
mixer_lock (mixer *m)
{
	pthread_mutex_lock (&(m->mutex));
}



static void
mixer_unlock (mixer *m)
{
	pthread_mutex_unlock (&(m->mutex));
}




static void *
mixer_main_thread (void *data)
{
	mixer *m = (mixer *) data;
	list *tmp, *tmp2;
	double time_slice;
	long slice_length, slice_spent;
	long wait_time = 0l;
	struct timeval start, end;
	double sum = 0.0;
	unsigned long count = 0;
	
	if (!m)
		return NULL;

	time_slice = (double) m->latency/88200;
	slice_length = time_slice*1000000;
	gettimeofday (&start, NULL);
	
	while (1) {
		mixer_lock (m);
		if (!m->running) {
			mixer_unlock (m);
			break;
		}
		if (m->notify_time > 0 && m->cur_time >= m->notify_time) {
		  pthread_cond_signal (&(m->notify_condition));
		  m->notify_time = -1.0;
		}
		for (tmp = m->busses; tmp; tmp = tmp->next) {
			MixerBus *b = (MixerBus *) tmp->data;

			if (b->enabled)
				mixer_bus_reset_data (b);
		}
		for (tmp = m->outputs; tmp; tmp = tmp->next) {
			MixerOutput *o = (MixerOutput *) tmp->data;
			if (o->enabled)
				mixer_output_reset_data (o);
		}
		tmp = m->channels;
		while (tmp) {
			MixerChannel *ch = (MixerChannel *) tmp->data;
			list *next = tmp->next;

			/* If this channel is disabled, skip it */

			if (!ch->enabled) {
				tmp = next;
				continue;
			}

			/* If this channel has no more data, kill it */

			if (ch->data_end_reached) {

				/* Get rid of this channel */

				m->channels = list_delete_item
					(m->channels, tmp);
				mixer_channel_destroy (ch);
				tmp = next;
				continue;
			}
			mixer_channel_get_data (ch);
			for (tmp2 = ch->patchpoints; tmp2; tmp2 = tmp2->next) {
				MixerPatchPoint  *p = (MixerPatchPoint *) tmp2->data;
				mixer_patch_point_post_data (p);
			}
			tmp = next;
		}
		for (tmp = m->busses; tmp; tmp = tmp->next) {
			MixerBus *b = (MixerBus *) tmp->data;
			if (b->enabled)
				mixer_bus_post_data (b);
		}
		for (tmp = m->outputs; tmp; tmp = tmp->next) {
			MixerOutput *o = (MixerOutput *) tmp->data;
			if (o->enabled)
				mixer_output_post_data (o);
		}
		m->cur_time += time_slice;
		mixer_unlock (m);
		gettimeofday (&end, NULL);
		slice_spent = (end.tv_sec-start.tv_sec)*1000000+
			(end.tv_usec-start.tv_usec);
		wait_time += slice_length-slice_spent;
		if (wait_time > 0)
			usleep (wait_time);
		start = end;
	}
	mixer_lock (m);
	m->thread = 0;
	mixer_unlock (m);
}



mixer *
mixer_new (int latency)
{
	mixer *m;

	m = (mixer *) malloc (sizeof(mixer));
	if (!m)
		return NULL;

	/* Setup mutex to protect mixer data */

	pthread_mutex_init (&(m->mutex), NULL);

	/* Setup notification condition */

	pthread_cond_init (&(m->notify_condition), NULL);
	m->notify_time = -1.0;

	/* Set mixer time to the current time */

	tzset ();
	m->cur_time = (double) time (NULL)+timezone+daylight*3600;

	/* No channels, busses, or outputs */

	m->channels = m->busses = m->outputs = NULL;

	/* Set mixer latency */

	m->latency = latency;

	m->running = 0;
	m->thread = 0;
	return m;
}



int
mixer_start (mixer *m)
{
	if (!m)
		return -1;
	mixer_lock (m);
	if (m->running || m->thread)
	{
		mixer_unlock (m);
		return -1;
	}

	/* Create mixer main thread */

	m->running = 1;
	if (pthread_create (&(m->thread),
			    NULL,
			    mixer_main_thread,
			    (void *) m))
	{
		m->running = 0;
		mixer_unlock (m);
		return -1;
	}
	mixer_unlock (m);
	return 0;
}



int
mixer_stop (mixer *m)
{
	pthread_t thread;

	if (!m)
		return -1;
	mixer_lock (m);
	if (!m->running)
	{
		mixer_unlock (m);
		return -1;
	}

	/* This could be incredibly broken */

	m->running = 0;
	thread = m->thread;
	mixer_unlock (m);
	pthread_join (thread, NULL);
	return 0;
}



void
mixer_destroy (mixer *m)
{
	list *tmp;

	if (!m)
		return;

	/* Stop the mixer to destroy it */
	
	mixer_stop (m);

	mixer_lock (m);

	/* Free output list */

	for (tmp = m->outputs; tmp; tmp = tmp->next)
		mixer_output_destroy ((MixerOutput *) tmp->data);
	list_free (m->outputs);

	/* Free Busses list */

	for (tmp = m->busses; tmp; tmp = tmp->next)
		mixer_bus_destroy ((MixerBus *) tmp->data);
	list_free (m->busses);

	/* Free channel list */

	for (tmp = m->channels; tmp; tmp = tmp->next)
		mixer_channel_destroy ((MixerChannel *) tmp->data);
	list_free (m->channels);

	mixer_unlock (m);
}



void
mixer_add_channel (mixer *m,
		   MixerChannel *ch)
{
	if (!m || !ch)
		return;
	mixer_lock (m);
	m->channels = list_prepend (m->channels, ch);
	mixer_unlock (m);
}



void
mixer_delete_channel (mixer *m,
		      const char *channel_name)
{
	list *tmp;

	if (!m)
		return;
	if (!channel_name)
		return;
	mixer_lock (m);
	for (tmp = m->channels; tmp; tmp = tmp->next)
	{
		MixerChannel *ch = (MixerChannel *) tmp->data;
		if (!strcmp (ch->name, channel_name))
			break;
	}
	if (tmp)
		m->channels = list_delete_item (m->channels, tmp);
	mixer_unlock (m);
}



void
mixer_enable_channel (mixer *m,
		      const char *channel_name,
		      int enabled)
{
	MixerChannel *ch;

	if (!m)
		return;
	ch = mixer_get_channel (m, channel_name);
	if (!ch)
		return;
	mixer_lock (m);
	ch->enabled = enabled;
	mixer_unlock (m);
}

MixerChannel *
mixer_get_channel (mixer *m,
		   const char *channel_name)
{
	list *tmp;

	if (!m)
		return NULL;
	mixer_lock (m);
	if (!m->channels)
	{
		mixer_unlock (m);
		return;
	}
	for (tmp = m->channels; tmp; tmp = tmp->next)
	{
		MixerChannel *ch = (MixerChannel *) tmp->data;

		if (!strcmp (channel_name, ch->name))
			break;
	}
	mixer_unlock (m);
	if (tmp)
		return (MixerChannel *) tmp->data;
	else
		return NULL;
}



void
mixer_add_bus (mixer *m,
	       MixerBus *b)
{
	if (!m)
		return;
	mixer_lock (m);
	m->busses = list_prepend (m->busses, b);
	mixer_unlock (m);
}




void
mixer_delete_bus (mixer *m,
		  const char *bus_name)
{
	list *tmp;

	if (!m)
		return;
	if (!bus_name)
		return;
	mixer_lock (m);
	for (tmp = m->busses; tmp; tmp = tmp->next)
	{
		MixerBus *b = (MixerBus *) tmp->data;
		if (!strcmp (b->name, bus_name))
			break;
	}
	if (tmp)
		m->busses = list_delete_item (m->busses, tmp);
	mixer_unlock (m);
}




MixerBus *
mixer_get_bus (mixer *m,
	       const char *bus_name)
{
	list *tmp;

	if (!m)
		return NULL;
	mixer_lock (m);
	if (!m->busses)
	{
		mixer_unlock (m);
		return NULL;
	}
	for (tmp = m->busses; tmp; tmp = tmp->next)
	{
		MixerBus *b = (MixerBus *) tmp->data;
		if (!strcmp (bus_name, b->name))
			break;
	}
	mixer_unlock (m);
	if (tmp)
		return (MixerBus *) tmp->data;
	else
		return NULL;
}



void
mixer_add_output (mixer *m,
		  MixerOutput *o)
{
	if (!m)
		return;
	mixer_lock (m);
	m->outputs = list_prepend (m->outputs, o);
	mixer_unlock (m);
}




void
mixer_delete_output (mixer *m,
		     const char *output_name)
{
	list *tmp;

	if (!m)
		return;
	if (!output_name)
		return;
	mixer_lock (m);
	for (tmp = m->outputs; tmp; tmp = tmp->next)
	{
		MixerOutput *o = (MixerOutput *) tmp->data;
		if (!strcmp (o->name, output_name))
			break;
	}
	if (tmp)
		m->outputs = list_delete_item (m->outputs, tmp);
	mixer_unlock (m);
}




void
mixer_enable_output (mixer *m,
		     const char *output_name,
		     int enabled)
{
	MixerOutput *o;

	if (!m)
		return;
	o = mixer_get_output (m, output_name);
	if (!o)
		return;
	mixer_lock (m);
	o->enabled = enabled;
	mixer_unlock (m);
}



MixerOutput *
mixer_get_output (mixer *m,
		  const char *output_name)
{
	list *tmp;

	if (!m)
		return NULL;
	mixer_lock (m);
	if (!m->outputs)
	{
		mixer_unlock (m);
		return NULL;
	}
	for (tmp = m->outputs; tmp; tmp = tmp->next)
	{
		MixerOutput *o = (MixerOutput *) tmp->data;

		if (!strcmp (output_name, o->name))
			break;
	}
	mixer_unlock (m);
	if (tmp)
		return (MixerOutput *) tmp->data;
	else
		return NULL;
}



void
mixer_patch_channel (mixer *m,
		     const char *channel_name,
		     const char *bus_name)
{
	MixerChannel *ch;
	MixerBus *b;
	MixerPatchPoint *p;
	
	if (!m)
		return;
	ch = mixer_get_channel (m, channel_name);
	b = mixer_get_bus (m, bus_name);

	if (!ch || !b)
		return;

	p = mixer_patch_point_new (ch, b, m->latency);
	mixer_lock (m);
	ch->patchpoints = list_prepend (ch->patchpoints, p);
	mixer_unlock (m);
}



void
mixer_patch_channel_all (mixer *m,
			 const char *channel_name)
{
	MixerChannel *ch;
	MixerBus *b;
	MixerPatchPoint *p;
	list *tmp;
	
	if (!m)
		return;
	ch = mixer_get_channel (m, channel_name);

	if (!ch)
		return;

	mixer_lock (m);
	if (ch->patchpoints) {
		list_free (ch->patchpoints);
		ch->patchpoints = NULL;
	}
	
	tmp = m->busses;
	while (tmp) {
		b = (MixerBus *) tmp->data;
		p = mixer_patch_point_new (ch, b, m->latency);
		ch->patchpoints = list_prepend (ch->patchpoints, p);
		tmp = tmp->next;
	}
	mixer_unlock (m);
}



void
mixer_delete_all_channels (mixer *m)
{
	list *tmp;

	if (!m)
		return;

	mixer_lock (m);
	tmp = m->channels;
	while (tmp)
	{
		list *next = tmp->next;
		MixerChannel *ch = (MixerChannel *) tmp->data;
		m->channels = list_delete_item (m->channels, tmp);
		mixer_channel_destroy (ch);
		tmp = next;
	}
	mixer_unlock (m);
}


void
mixer_patch_bus (mixer *m,
		 const char *bus_name,
		 const char *output_name)
{
	MixerBus *b;
	MixerOutput *o;
  
	if (!m)
		return;
	b = mixer_get_bus (m, bus_name);
	o = mixer_get_output (m, output_name);
  
	if (!b || !o)
		return;

	mixer_lock (m);
	b->outputs = list_prepend (b->outputs, o);
	mixer_unlock (m);
}



double
mixer_get_time (mixer *m)
{
	double rv;

	if (!m)
		return 0.0;
	mixer_lock (m);
	rv = m->cur_time;
	mixer_unlock (m);
	return rv;
}



void
mixer_sync_time (mixer *m)
{
	time_t cur_time = time (NULL);

	if (!m)
		return;

	/* Wait for time to change */

	while (cur_time == time (NULL));

	cur_time = time (NULL);
	mixer_lock (m);
	m->cur_time = cur_time;
	mixer_unlock (m);
}



void
mixer_fade_channel (mixer *m,
		    const char *channel_name,
		    double fade_destination,
		    double fade_time)
{
	MixerChannel *ch;
	double fade_distance;

	if (!m)
		return;

	ch = mixer_get_channel (m, channel_name);

	if (!ch)
		return;

	mixer_lock (m);
	if (ch->fade != 0.0)
	{
		mixer_unlock (m);
		return;
	}
	fade_distance = fade_destination-(ch->level);
	ch->fade = (fade_distance/fade_time)/ch->rate;
	ch->fade_destination = fade_destination;
	mixer_unlock (m);
}



void
mixer_fade_all (mixer *m,
		double level,
		double fade_time)
{
	MixerChannel *ch;
	double fade_distance;
	list *tmp;

	if (!m)
		return;

	mixer_lock (m);
	for (tmp = m->channels; tmp; tmp = tmp->next)
	{
		ch = (MixerChannel *) tmp->data;
		if (!ch)
			continue;
		fade_distance = level-(ch->level);
		ch->fade = (fade_distance/fade_time)/ch->rate;
		ch->fade_destination = level;
	}
	mixer_unlock (m);
}



void
mixer_reset_notification_time (mixer *m,
			       double notify_time)
{
	mixer_lock (m);
	m->notify_time = notify_time;
	mixer_unlock (m);
}




void
mixer_wait_for_notification (mixer *m,
			     double notify_time)
{
	if (!m)
		return;
	mixer_lock (m);
	if (m->notify_time > 0 && notify_time < m->cur_time) {
		mixer_unlock (m);
		return;
	}
	m->notify_time = notify_time;
	pthread_cond_wait (&(m->notify_condition), &(m->mutex));
	mixer_unlock (m);
}
