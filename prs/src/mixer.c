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
 * mixer.c: Implementation of the mixer object.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <malloc.h>
#include "debug.h"
#include "mixer.h"
#include "mixerchannel.h"
#include "mixerbus.h"
#include "mixerpatchpoint.h"
#include "mixeroutput.h"
#include "vorbismixerchannel.h"
#include "mp3mixerchannel.h"



typedef MixerChannel *(*create_func)(const char *name, const char *path, int latency);



typedef struct {
	char *extension;
	create_func constructor;
} channel_type_constructor_link;


#define N_LINKS 2

channel_type_constructor_link type_links[N_LINKS] = {
	{".mp3", mp3_mixer_channel_new},
	{".ogg", vorbis_mixer_channel_new}
};


static void
mixer_lock (mixer *m)
{
	assert (m != NULL);
	pthread_mutex_lock (&(m->mutex));
}



static void
mixer_unlock (mixer *m)
{
	assert (m != NULL);
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
	
	assert (m != NULL);
	debug_printf (DEBUG_FLAGS_MIXER, "mixer main thread starting\n");
	time_slice = (double) m->latency/44100;
	slice_length = time_slice*1000000;
	gettimeofday (&start, NULL);
	
	while (1) {
		mixer_lock (m);
		if (!m->running) {
			mixer_unlock (m);
			break;
		}
		if (m->notify_time > 0 && m->cur_time >= m->notify_time) {
			debug_printf (DEBUG_FLAGS_MIXER,
				      "mixer notification time has come\n");
			pthread_cond_signal (&(m->notify_condition));
			m->notify_time = -1.0;
		}
		for (tmp = m->busses; tmp; tmp = tmp->next) {
			MixerBus *b = (MixerBus *) tmp->data;
			assert (b != NULL);

			if (b->enabled)
				mixer_bus_reset_data (b);
		}
		for (tmp = m->outputs; tmp; tmp = tmp->next) {
			MixerOutput *o = (MixerOutput *) tmp->data;
			assert (o != NULL);
			if (o->enabled)
				mixer_output_reset_data (o);
		}
		tmp = m->channels;
		while (tmp) {
			MixerChannel *ch = (MixerChannel *) tmp->data;
			list *next = tmp->next;
			assert (ch != NULL);

			/* If this channel is disabled, skip it */

			if (!ch->enabled || !ch->patchpoints) {
				tmp = next;
				continue;
			}

			/* If this channel has no more data, kill it */

			if (ch->data_end_reached) {

				/* Get rid of this channel */

				debug_printf (DEBUG_FLAGS_MIXER,
					      "mixer: end of data for %s\n",
					      ch->name);
				m->channels = list_delete_item
					(m->channels, tmp);
				mixer_channel_destroy (ch);
				tmp = next;
				continue;
			}
			if (ch->data_reader_thread == 0) {
				int rv = mixer_channel_get_data (ch);
				if (rv < ch->chunk_size)
					ch->data_end_reached = 1;
			}
			mixer_channel_process_levels (ch);
			for (tmp2 = ch->patchpoints; tmp2; tmp2 = tmp2->next) {
				MixerPatchPoint  *p = (MixerPatchPoint *) tmp2->data;
				assert (p != NULL);
				mixer_patch_point_post_data (p);
			}
			mixer_channel_advance_pointers (ch);
			tmp = next;
		}
		for (tmp = m->busses; tmp; tmp = tmp->next) {
			MixerBus *b = (MixerBus *) tmp->data;
			assert (b != NULL);
			if (b->enabled)
				mixer_bus_post_data (b);
		}
		for (tmp = m->outputs; tmp; tmp = tmp->next) {
			MixerOutput *o = (MixerOutput *) tmp->data;
			assert (o != NULL);
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
	debug_printf (DEBUG_FLAGS_MIXER, "mixer main thread exiting\n");
}



mixer *
mixer_new (int latency)
{
	mixer *m;

	debug_printf (DEBUG_FLAGS_MIXER,
		      "mixer_new (%d)\n", latency);
	assert (latency > 0);
	m = (mixer *) malloc (sizeof(mixer));
	assert (m != NULL);

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
	m->default_level = 1.0;
	return m;
}



int
mixer_start (mixer *m)
{
	assert (m != NULL);
	mixer_lock (m);
	if (m->running || m->thread)
	{
		debug_printf (DEBUG_FLAGS_MIXER,
			      "mixer_start: mixer already running\n");
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
		debug_printf (DEBUG_FLAGS_MIXER,
			      "mixer_start: failed to create thread\n");
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

	assert (m != NULL);
	mixer_lock (m);
	if (!m->running)
	{
		debug_printf (DEBUG_FLAGS_MIXER,
			      "mixer_stop: mixer isn't running\n");
		mixer_unlock (m);
		return -1;
	}

	/* This could be incredibly broken */

	debug_printf (DEBUG_FLAGS_MIXER, "stopping mixer\n");
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

	assert (m != NULL);

	/* Stop the mixer to destroy it */
	
	mixer_stop (m);
	debug_printf (DEBUG_FLAGS_MIXER, "destroying mixer\n");
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
	assert (m != NULL);
	assert (ch != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "adding channel %s to mixer\n", ch->name);
	mixer_lock (m);
	m->channels = list_prepend (m->channels, ch);
	mixer_unlock (m);
}



void
mixer_delete_channel (mixer *m,
		      const char *channel_name)
{
	list *tmp;

	assert (m != NULL);
	assert (channel_name != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "deleting channel %s from mixer\n", channel_name);
	mixer_lock (m);
	for (tmp = m->channels; tmp; tmp = tmp->next)
	{
		MixerChannel *ch = (MixerChannel *) tmp->data;
		assert (ch != NULL);
		if (!strcmp (ch->name, channel_name))
			break;
	}
	if (tmp)
		m->channels = list_delete_item (m->channels, tmp);
	else
		debug_printf (DEBUG_FLAGS_MIXER,
			      "mixer_delete_channel: %s not found\n",
			      channel_name);
	mixer_unlock (m);
}



void
mixer_enable_channel (mixer *m,
		      const char *channel_name,
		      int enabled)
{
	MixerChannel *ch;

	assert (m != NULL);
	assert (channel_name != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "attempting to enable channel %s\n", channel_name);
	ch = mixer_get_channel (m, channel_name);
	if (!ch)
		return;
	mixer_lock (m);
	if (enabled && m->default_level != 1.0)
		ch->level = m->default_level;
	ch->enabled = enabled;
	mixer_unlock (m);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "channel %s enabled\n", channel_name);
}

MixerChannel *
mixer_get_channel (mixer *m,
		   const char *channel_name)
{
	list *tmp;

	assert (m != NULL);
	mixer_lock (m);
	if (!m->channels)
	{
		debug_printf (DEBUG_FLAGS_MIXER,
			      "mixer_get_channel: no channels\n");
		mixer_unlock (m);
		return;
	}
	for (tmp = m->channels; tmp; tmp = tmp->next)
	{
		MixerChannel *ch = (MixerChannel *) tmp->data;
		assert (ch != NULL);

		if (!strcmp (channel_name, ch->name))
			break;
	}
	mixer_unlock (m);
	if (tmp)
		return (MixerChannel *) tmp->data;
	else {
		debug_printf (DEBUG_FLAGS_MIXER,
			      "mixer_get_channel: %s not found\n",
			      channel_name);
		return NULL;
	}
}



void
mixer_add_bus (mixer *m,
	       MixerBus *b)
{
	assert (m != NULL);
	assert (b != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "adding mixer bus %s\n", b->name);
	mixer_lock (m);
	m->busses = list_prepend (m->busses, b);
	mixer_unlock (m);
}




void
mixer_delete_bus (mixer *m,
		  const char *bus_name)
{
	list *tmp;

	assert (m != NULL);
	assert (bus_name != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "attempting to delete mixer bus %s\n", bus_name);
	mixer_lock (m);
	for (tmp = m->busses; tmp; tmp = tmp->next)
	{
		MixerBus *b = (MixerBus *) tmp->data;
		assert (b != NULL);
		if (!strcmp (b->name, bus_name))
			break;
	}
	if (tmp) {
		m->busses = list_delete_item (m->busses, tmp);
		debug_printf (DEBUG_FLAGS_MIXER,
			      "deleted bus %s\n", bus_name);
	} else
		debug_printf (DEBUG_FLAGS_MIXER,
			      "mixer_delete_bus: %s not found\n", bus_name);
	mixer_unlock (m);
}




MixerBus *
mixer_get_bus (mixer *m,
	       const char *bus_name)
{
	list *tmp;

	assert (m != NULL);
	assert (bus_name != NULL);
	mixer_lock (m);
	if (!m->busses)
	{
		debug_printf (DEBUG_FLAGS_MIXER,
			      "mixer_get_bus: no busses\n");
		mixer_unlock (m);
		return NULL;
	}
	for (tmp = m->busses; tmp; tmp = tmp->next)
	{
		MixerBus *b = (MixerBus *) tmp->data;
		assert (b != NULL);
		if (!strcmp (bus_name, b->name))
			break;
	}
	mixer_unlock (m);
	if (tmp)
		return (MixerBus *) tmp->data;
	else {
		debug_printf (DEBUG_FLAGS_MIXER,
			      "mixer_get_bus: %s not found\n", bus_name);
		return NULL;
	}
}



void
mixer_add_output (mixer *m,
		  MixerOutput *o)
{
	assert (m != NULL);
	assert (o != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "adding mixer output %s\n", o->name);
	mixer_lock (m);
	m->outputs = list_prepend (m->outputs, o);
	mixer_unlock (m);
}




void
mixer_delete_output (mixer *m,
		     const char *output_name)
{
	list *tmp;

	assert (m != NULL);
	assert (output_name != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "attempting to delete mixer output %s\n", output_name);
	mixer_lock (m);
	for (tmp = m->outputs; tmp; tmp = tmp->next)
	{
		MixerOutput *o = (MixerOutput *) tmp->data;
		assert (o != NULL);
		if (!strcmp (o->name, output_name))
			break;
	}
	if (tmp) {
		m->outputs = list_delete_item (m->outputs, tmp);
		debug_printf (DEBUG_FLAGS_MIXER,
			      "deleted mixer output %s\n", output_name);
	} else
		debug_printf (DEBUG_FLAGS_MIXER,
			      "mixer_delete_output: %s not found\n",
			      output_name);
	mixer_unlock (m);
}




void
mixer_enable_output (mixer *m,
		     const char *output_name,
		     int enabled)
{
	MixerOutput *o;

	assert (m != NULL);
	assert (output_name != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "attempting to enable mixer output %s\n", output_name);
	o = mixer_get_output (m, output_name);
	if (!o)
		return;
	mixer_lock (m);
	o->enabled = enabled;
	debug_printf (DEBUG_FLAGS_MIXER,
		      "enabled mixer output %s\n", o->name);
	mixer_unlock (m);
}



MixerOutput *
mixer_get_output (mixer *m,
		  const char *output_name)
{
	list *tmp;

	assert (m != NULL);
	assert (output_name != NULL);
	mixer_lock (m);
	if (!m->outputs)
	{
		debug_printf (DEBUG_FLAGS_MIXER,
			      "mixer_get_output: no outputs\n");
		mixer_unlock (m);
		return NULL;
	}
	for (tmp = m->outputs; tmp; tmp = tmp->next)
	{
		MixerOutput *o = (MixerOutput *) tmp->data;
		assert (o != NULL);

		if (!strcmp (output_name, o->name))
			break;
	}
	mixer_unlock (m);
	if (tmp)
		return (MixerOutput *) tmp->data;
	else {
		debug_printf (DEBUG_FLAGS_MIXER,
			      "mixer_get_output: %s not found\n", output_name);
		return NULL;
	}
}



void
mixer_patch_channel (mixer *m,
		     const char *channel_name,
		     const char *bus_name)
{
	MixerChannel *ch;
	MixerBus *b;
	MixerPatchPoint *p;
	
	assert (m != NULL);
	assert (channel_name != NULL);
	assert (bus_name != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "attempting to patch: channel=%s, bus=%s\n",
		      channel_name, bus_name);
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
	
	assert (m != NULL);
	assert (channel_name != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "attempting to patch channel %s to all busses\n",
		      channel_name);
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
		assert (b != NULL);
		p = mixer_patch_point_new (ch, b, m->latency);
		ch->patchpoints = list_prepend (ch->patchpoints, p);
		tmp = tmp->next;
	}
	mixer_unlock (m);
}



void
mixer_delete_channels (mixer *m,
		       const int key)
{
	list *tmp;

	assert (m != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "deleting all mixer channels\n");
	mixer_lock (m);
	tmp = m->channels;
	while (tmp) {
		list *next = tmp->next;
		MixerChannel *ch = (MixerChannel *) tmp->data;
		assert (ch != NULL);
		if (ch->key == key) {
			m->channels = list_delete_item (m->channels, tmp);
			debug_printf (DEBUG_FLAGS_MIXER,
				      "deleted channel %s\n", ch->name);
			mixer_channel_destroy (ch);
		}
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
  
	assert (bus_name != NULL);
	assert (output_name != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "attempting to patch: bus=%s, output=%s\n",
		      bus_name, output_name);
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

	assert (m != NULL);
	mixer_lock (m);
	rv = m->cur_time;
	mixer_unlock (m);
	return rv;
}



void
mixer_sync_time (mixer *m)
{
	struct timeval v;

	assert (m != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "mixer_sync_time called\n");

	mixer_lock (m);
	gettimeofday (&v, NULL);
	m->cur_time = v.tv_sec+(v.tv_usec/1000000);
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

	assert (m != NULL);
	assert (channel_name != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "fading channel %s: destionation=%f, time=%f\n",
		      channel_name, fade_destination, fade_time);

	ch = mixer_get_channel (m, channel_name);

	if (!ch)
		return;

	mixer_lock (m);
	ch->fade = pow ((fade_destination+.001)/(ch->level+.001), 1/(ch->rate*fade_time));
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

	assert (m != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "fading all channels: level=%f, time=%f\n",
		      level, fade_time);

	mixer_lock (m);
	for (tmp = m->channels; tmp; tmp = tmp->next) {
		ch = (MixerChannel *) tmp->data;
		assert (ch != NULL);
		if (ch->enabled) {
			ch->fade = pow ((level+.001)/(ch->level+.001), 1.0/(ch->rate*fade_time));
			ch->fade_destination = level;
		}
	}
	mixer_unlock (m);
}



void
mixer_reset_notification_time (mixer *m,
			       double notify_time)
{
	assert (m != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "mixer_reset_notification_time: notify_time=%f\n",
		      notify_time);
	mixer_lock (m);
	m->notify_time = notify_time;
	mixer_unlock (m);
}




void
mixer_wait_for_notification (mixer *m,
			     double notify_time)
{
	assert (m != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "mixer_wait_for_notification: notify_time=%f\n",
		      notify_time);
	mixer_lock (m);
	if (m->notify_time > 0 && notify_time < m->cur_time) {
		mixer_unlock (m);
		return;
	}
	m->notify_time = notify_time;
	pthread_cond_wait (&(m->notify_condition), &(m->mutex));
	mixer_unlock (m);
}


void
mixer_set_default_level (mixer *m,
			 double level)
{
	mixer_lock (m);
	m->default_level = level;
	mixer_unlock (m);
}


void
mixer_add_file (mixer *m,
		const char *channel_name,
		const char *path,
		const int key)
{
	MixerChannel *ch = NULL;
	const char *file_extension;
	int i;
	
	file_extension = path+strlen(path)-4;

	for (i = 0; i < N_LINKS; i++) {
		if (!strcmp (type_links[i].extension, file_extension))
			ch = type_links[i].constructor (channel_name, path, m->latency);
	}
	if (ch) {
		ch->key = key;
		mixer_add_channel (m, ch);
	}
}
