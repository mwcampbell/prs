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
 * mixerautomation.c: Implementation of the mixer automation object.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <signal.h>
#include "debug.h"
#include "mixerautomation.h"
#include "db.h"
#include "vorbismixerchannel.h"
#include "mp3mixerchannel.h"



AutomationEvent *
automation_event_new (void)
{
	AutomationEvent *e = (AutomationEvent *) malloc (sizeof (AutomationEvent));

	if (!e)
		return NULL;
	e->type = AUTOMATION_EVENT_TYPE_UNDEFINED;
	e->delta_time = 0.0;
	e->level = 1.0;
	e->channel_name = NULL;
	e->detail1 = e->detail2 = NULL;
	e->data = -1;
	return e;
}



void
automation_event_destroy (AutomationEvent *e)
{
	if (!e)
		return;
	if (e->channel_name)
		free (e->channel_name);
	if (e->detail1)
		free (e->detail1);
	if (e->detail2)
		free (e->detail2);
	free (e);
}



MixerAutomation *
mixer_automation_new (mixer *m, Database *db)
{
	MixerAutomation *a = (MixerAutomation *) malloc (sizeof (MixerAutomation));
	a->m = m;
	a->db = db;
	a->events = NULL;
	a->last_event_time = mixer_get_time (m);
	a->automation_thread = 0;
	a->running = 0;
	a->loggers = NULL;
	pthread_mutex_init (&(a->mut), NULL);
	debug_printf (DEBUG_FLAGS_AUTOMATION, "Automation object created.\n");
	return a;
}



void
mixer_automation_destroy (MixerAutomation *a)
{
	list *tmp;
	logger *l;
		if (!a)		return;
	if (a->automation_thread > 0) {
		mixer_reset_notification_time (a->m, mixer_get_time (a->m));
		a->running = 0;
		pthread_join (a->automation_thread, NULL);
	}
	for (tmp = a->events; tmp; tmp = tmp->next)
	{
		automation_event_destroy ((AutomationEvent *) tmp->data);
	}
	for (tmp = a->loggers; tmp; tmp = tmp->next) {
		l = (logger *) tmp->data;
		logger_destroy (l);
	}
	if (a->loggers)
		list_free (a->loggers);
	free (a);
	debug_printf (DEBUG_FLAGS_AUTOMATION, "Automation object destroyed.\n");
}



int
mixer_automation_add_event (MixerAutomation *a,
			    AutomationEvent *e)
{
	if (!a)
		return -1;
	if (!e)
		return -1;
	if (e->delta_time < 0)
		e->delta_time = 0;
	pthread_mutex_lock (&(a->mut));
	if (!a->events)
		if (a->running) {
			mixer_reset_notification_time (a->m, a->last_event_time+e->delta_time);
		}
	a->events = list_append (a->events, e);
	pthread_mutex_unlock (&(a->mut));
	switch (e->type) {
	case AUTOMATION_EVENT_TYPE_ENABLE_CHANNEL:
			debug_printf (DEBUG_FLAGS_AUTOMATION, "Enable channel event added %s\n", e->channel_name);
			break;
	case AUTOMATION_EVENT_TYPE_FADE_CHANNEL:
		debug_printf (DEBUG_FLAGS_AUTOMATION, "fade channel event added %s\n", e->channel_name);
		break;
	case AUTOMATION_EVENT_TYPE_FADE_ALL:
		debug_printf (DEBUG_FLAGS_AUTOMATION, "fade all event added\n");
		break;
	}
}



void
mixer_automation_next_event (MixerAutomation *a)
{
	AutomationEvent *e;
	MixerChannel *ch;      
	double mixer_time;
	list *tmp;
	logger *l;

	if (!a)
		return;
	pthread_mutex_lock (&(a->mut));
	if (a->events)
		e = (AutomationEvent *) a->events->data;
	else {
		pthread_mutex_unlock (&(a->mut));
		debug_printf (DEBUG_FLAGS_AUTOMATION, "mixer_Automation_next_event called with no events on stack.\n");
		return;
	}

	mixer_time = mixer_get_time (a->m);
	debug_printf (DEBUG_FLAGS_AUTOMATION, "Executing automation event, time %lf lag is %lf\n", mixer_time, a->last_event_time+e->delta_time-mixer_time);
    
        /* Do event */

	switch (e->type)
	{
	case AUTOMATION_EVENT_TYPE_ENABLE_CHANNEL:
		ch = mixer_get_channel (a->m, e->channel_name);
		if (ch) {
			ch->level = e->level;
			
			mixer_patch_channel_all (a->m, e->channel_name);
			mixer_enable_channel (a->m, e->channel_name, 1);
			for (tmp = a->loggers; tmp; tmp = tmp->next) {
				l = (logger *) tmp->data;
				logger_log_file (l, ch->location);
			}
		}
		break;
	case AUTOMATION_EVENT_TYPE_FADE_CHANNEL:
		mixer_fade_channel (a->m, e->channel_name, e->level, atof (e->detail1));
		break;
	case AUTOMATION_EVENT_TYPE_FADE_ALL:
		mixer_fade_all (a->m, e->level, e->length);
		break;
	case AUTOMATION_EVENT_TYPE_DELETE_CHANNELS:
		mixer_delete_channels (a->m, e->data);
		break;
	}
	a->events = list_delete_item (a->events, a->events);
	if (a->running)
		a->last_event_time = a->last_event_time+e->delta_time;
	else
		a->last_event_time = mixer_time;
	automation_event_destroy (e);
	pthread_mutex_unlock (&(a->mut));
}




static void *
mixer_automation_main_thread (void *data)
{
	MixerAutomation *a = (MixerAutomation *) data;

	while (1)
	{

		/* Wait time defaults to wait for like thirty years in the case where
		 * we have no event in the queue
		 */

		double wait_time = (double) (0x7fffffff);
		AutomationEvent *e;

		pthread_mutex_lock (&(a->mut));
		if (a->events)
			e = (AutomationEvent *) a->events->data;
		else
			e = NULL;

		/* If there's an event at the top of the queue, set the mixer wait
		 * notification to notify us when it's time has been reached
		 */

		if (e)
		{
			wait_time = a->last_event_time+e->delta_time;
		}
      
		/* If the automation isn't running, bail */

		if (!a->running)
		{
			pthread_mutex_unlock (&(a->mut));
			break;
		}
      
		/* Wait for the mixer to notify us that the specified time has been reached */

		pthread_mutex_unlock (&(a->mut));
		mixer_wait_for_notification (a->m, wait_time);
		/* If someone turned automation off while we were waiting, bail now */

		pthread_mutex_lock (&(a->mut));
		if (!a->running) {
			pthread_mutex_unlock (&(a->mut));
			break;
		}
		pthread_mutex_unlock (&(a->mut));
		mixer_automation_next_event (a);
	}
	pthread_mutex_lock (&(a->mut));
	a->automation_thread = 0;
	pthread_mutex_unlock (&(a->mut));
	debug_printf (DEBUG_FLAGS_AUTOMATION, "Automation thread exitting.\n");
}



int
mixer_automation_start (MixerAutomation *a)
{
	AutomationEvent *e;
  
	if (!a) {
		debug_printf (DEBUG_FLAGS_AUTOMATION, "mixer_automation_start called with null automation pointer\n");
		return -1;
	}
	pthread_mutex_lock (&(a->mut));
	if (a->running) {
		pthread_mutex_unlock (&(a->mut));
		debug_printf (DEBUG_FLAGS_AUTOMATION, "mixer_automation-start called on a running automation object\n");
		return -1;
	}
	a->running = 1;
	pthread_create (&(a->automation_thread), NULL, mixer_automation_main_thread, (void *) a);
	if (a->events) {
		e = (AutomationEvent *) a->events->data;
		if (a->last_event_time+e->delta_time < mixer_get_time (a->m)) {
			a->last_event_time = mixer_get_time (a->m);
			e->delta_time = 0;
			mixer_reset_notification_time (a->m, a->last_event_time+e->delta_time);
		}
	}
	pthread_mutex_unlock (&(a->mut));
	return 0;
}



int
mixer_automation_stop (MixerAutomation *a)
{
	pthread_t thread;
  
	if (!a) {
		debug_printf (DEBUG_FLAGS_AUTOMATION, "mixer_automation_stop called with null automation pointer\n");
		return -1;
	}
	pthread_mutex_lock (&(a->mut));
	if (!a->running) {
		pthread_mutex_unlock (&(a->mut));
		debug_printf (DEBUG_FLAGS_AUTOMATION, "mixer_automation_stop called with already stopped automation object\n");
		return -1;
	}
	mixer_reset_notification_time (a->m, mixer_get_time (a->m));
	a->running = 0;
	thread = a->automation_thread;
	pthread_mutex_unlock (&(a->mut));
	debug_printf (DEBUG_FLAGS_AUTOMATION, "mixer_automation_stop Joining automation thread\n");
	pthread_join (thread, NULL);
	debug_printf (DEBUG_FLAGS_AUTOMATION, "mixer_automation_stop done Joining automation thread\n");
	return 0;
}



void
mixer_automation_set_start_time (MixerAutomation *a,
				 double start_time)
{
	AutomationEvent *e;
	list *tmp;
	double cur_time = a->last_event_time;
	time_t ct = (time_t) start_time;
	
	debug_printf (DEBUG_FLAGS_AUTOMATION, "Resetting automation start time to %s", ctime (&ct));
	if (!a) {
		debug_printf (DEBUG_FLAGS_AUTOMATION, "mixer_automation_set_start_time called with null automation object\n");
		return;
	}
	pthread_mutex_lock (&(a->mut));
	tmp = a->events;
	while (tmp) {
		list *next = tmp->next;
		AutomationEvent *e = tmp->data;
		cur_time += e->delta_time;
		if (cur_time < start_time) {
			debug_printf (DEBUG_FLAGS_AUTOMATION, "Removing event %lf seconds before start time\n", start_time-cur_time);
			automation_event_destroy (e);
			a->events = list_delete_item (a->events, tmp);
		}
		else
			break;
		tmp = next;
	}
	if (a->events) {
		e = (AutomationEvent *) a->events->data;
		e->delta_time -= start_time-a->last_event_time;
		mixer_reset_notification_time (a->m, start_time+e->delta_time);
	}
	else
		mixer_reset_notification_time (a->m, -1l);
	a->last_event_time = start_time;
	pthread_mutex_unlock (&(a->mut));
}



double
mixer_automation_get_last_event_end (MixerAutomation *a)
{
	double event_start_time, event_end_time, rv;
	list *tmp;
	time_t lt;
  
	if (!a)
		return -1.0;

	pthread_mutex_lock (&(a->mut));
	rv = event_start_time = event_end_time = a->last_event_time;
	lt = rv;
	for (tmp = a->events; tmp; tmp = tmp->next)
	{
		AutomationEvent *e = (AutomationEvent *) tmp->data;
		time_t st, et;
      
		event_start_time += e->delta_time;
		event_end_time = event_start_time + e->length;
		st = event_start_time;
		et = event_end_time;
		if (event_end_time > rv)
			rv = event_end_time;
	}
	pthread_mutex_unlock (&(a->mut));
	return rv;
}


void
mixer_automation_add_logger (MixerAutomation *a,
			     logger *l)
{
	if (!a || !l)
		return;
	pthread_mutex_lock (&(a->mut));
	a->loggers = list_append (a->loggers, l);
	a->logger_enabled = 1;
	pthread_mutex_unlock (&(a->mut));
}


void
mixer_automation_enable_logger (MixerAutomation *a,
				int enabled)
{
	if (!a)
		return;
	pthread_mutex_lock (&(a->mut));
	a->logger_enabled = enabled;
	pthread_mutex_unlock (&(a->mut));
}
