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
 * scheduler.c: Implementation of the scheduler object.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <signal.h>
#include "debug.h"
#include "scheduler.h"
#include "db.h"
#include "fileinfo.h"
#include "urlmixerchannel.h"



typedef struct {
	PlaylistTemplate *t;
	int event_number;
	int length;
	RecordingPicker *p;
} template_stack_entry;



typedef struct {
	mixer *m;
	MixerAutomation *a;
	
	/* Url parameters */

	char *url;
	char *archive_file_name;
	double last_event_start_time;
	double start_time;
	double end_time;
	double retry_delay;
	double end_fade;
} url_manager_info;


	
static void
scheduler_push_template (scheduler *s,
			 PlaylistTemplate *t,
			 int event_number)
{
	template_stack_entry *e = (template_stack_entry *) 
		malloc (sizeof (template_stack_entry));
	e->t = t;
	e->event_number = event_number;
	e->length = list_length (t->events);
	e->p = recording_picker_new (s->db, t->artist_exclude, t->recording_exclude);
	s->template_stack = list_prepend (s->template_stack, e);
	debug_printf (DEBUG_FLAGS_SCHEDULER, "Pushing template %s on the scheduler stack\n", t->name);
}



static void
scheduler_pop_template (scheduler *s)
{
	template_stack_entry *e;

	if (!s)
		return;
	if (!s->template_stack) {
		debug_printf (DEBUG_FLAGS_SCHEDULER, "No templates to pop off scheduler stack\n");
		return;
		}
	e = (template_stack_entry *) s->template_stack->data;
	debug_printf (DEBUG_FLAGS_SCHEDULER, "Popping template %s off scheduler's stack\n", e->t->name);
	playlist_template_destroy (e->t);
	recording_picker_destroy (e->p);
	free (e);
	s->template_stack = list_delete_item (s->template_stack, s->template_stack);
}



scheduler *
scheduler_new (MixerAutomation *a, Database *db, double cur_time)
{
	scheduler *s = (scheduler *) malloc (sizeof (scheduler));
	s->a = a;
	s->db = db;
	s->template_stack = NULL;
	s->last_event_end_time =
		s->prev_event_start_time =
		s->prev_event_end_time = cur_time;
	mixer_automation_set_start_time (a, cur_time);
	s->scheduler_thread = 0;
	s->running = 0;
	s->preschedule = 0.0;
	pthread_mutex_init (&(s->mut), NULL);
	debug_printf (DEBUG_FLAGS_SCHEDULER, "Creating scheduler object");
	return s;
}



void
scheduler_destroy (scheduler *s)
{
	pthread_t thread;

	if (!s)
		return;
	pthread_mutex_lock (&(s->mut));
	s->running = 0;
	thread = s->scheduler_thread;
	pthread_mutex_unlock (&(s->mut));
		list_free (s->template_stack);
	free (s);
debug_printf (DEBUG_FLAGS_SCHEDULER, "Destroying scheduler object\n");
}



static void
scheduler_switch_templates (scheduler *s)
{
	PlaylistTemplate *t;
	template_stack_entry *e;
	double start_time;

	if (!s)
		return;

	/* What's on the top of the stack */

	if (s->template_stack)
		e = (template_stack_entry *) s->template_stack->data;
	else
		e = NULL;

	/* Get the current template */
	
	t = get_playlist_template (s->db, s->last_event_end_time);

	if (t && e && s->prev_event_end_time <= e->t->end_time &&
	    (t->id == e->t->id || e->t->type == TEMPLATE_TYPE_FALLBACK))
		return;

	if (e) {
		double start_time;
		AutomationEvent *ae = automation_event_new ();
		double start_delta;

		if (s->prev_event_end_time >= e->t->end_time)
			start_time = e->t->end_time-e->t->end_prefade;
		else
			start_time = mixer_get_time (s->a->m);
		start_delta = start_time-s->prev_event_start_time;
		ae->type = AUTOMATION_EVENT_TYPE_FADE_ALL;
		ae->delta_time = start_delta;
		ae->length = e->t->end_prefade;
		ae->level = 0;
		mixer_automation_add_event (s->a, ae);
		if (start_time < e->t->end_time)
			mixer_automation_set_start_time (s->a, start_time);
		s->prev_event_start_time = start_time;
		s->last_event_end_time = s->prev_event_end_time = start_time+e->t->end_prefade;
		scheduler_pop_template (s);
		if (s->template_stack)
			return;
	}

	if (!t && !e) {
		s->last_event_end_time += s->preschedule;
		s->prev_event_end_time = s->last_event_end_time;
	}
	if (t) {

		/* Set new template */

		if (!e)
			start_time = mixer_get_time (s->a->m);
		else
			start_time = t->start_time;
		if (t->start_time > start_time)
			start_time = t->start_time;
		s->last_event_end_time = s->prev_event_end_time = start_time;
		if (!e) {
			s->prev_event_start_time = start_time;
			mixer_automation_set_start_time (s->a, start_time);
		}
		t->start_time = start_time;
		scheduler_push_template (s, t, 1);
	}
}



static void *
url_manager (void *data)
{
	url_manager_info *i = (url_manager_info *) data;
	double cur_time;
	unsigned int retry_sleep_delay = (unsigned long) (i->retry_delay)*1000000;
	double start_delay;
	MixerChannel *ch;
	
	/* Wait until start time */

	cur_time = i->start_time;

	start_delay = i->start_time;
	start_delay -= mixer_get_time (i->m);
	if (start_delay > 0)
		usleep ((unsigned int) start_delay*1000000);
	
	cur_time = mixer_get_time (i->m);
	mixer_automation_stop (i->a);
	
	/* Try an initial attempt */

	ch = url_mixer_channel_new (i->url, i->url,
				    i->archive_file_name,
				    i->m->latency);
			
	if (ch) {
		ch->level = 0;
		ch->enabled = 1;
		mixer_add_channel (i->m, ch);
		mixer_patch_channel_all (i->m, i->url);
		mixer_automation_stop (i->a);
		mixer_fade_all (i->m, 0, 1);
		mixer_fade_channel (i->m, i->url, 1, 1);
	}
	while (cur_time < i->end_time-i->end_fade) {
		ch = mixer_get_channel (i->m, i->url);
		if (!ch) {

			/* Turn automation on */

			mixer_fade_all (i->m, 1.0, 1);
			mixer_automation_start (i->a);

			/* Try to create the URL channel */

			ch = url_mixer_channel_new (i->url, i->url,
						    i->archive_file_name,
						    i->m->latency);
			
			if (ch) {
				ch->level = 0;
				ch->enabled = 1;
				mixer_add_channel (i->m, ch);
				mixer_patch_channel_all (i->m, i->url);
			}
		}
		else
			ch = NULL;
		usleep (retry_sleep_delay);
		if (ch && mixer_get_channel (i->m, i->url)) {
			mixer_automation_stop (i->a);
			mixer_fade_all (i->m, 0, 1);
			mixer_fade_channel (i->m, i->url, 1, 1);
		}
		cur_time = mixer_get_time (i->m);
	}
	
	/* Ensure mixer automation is running when we leave */

	mixer_automation_stop (i->a);
	mixer_automation_set_start_time (i->a, i->end_time-i->end_fade);
	mixer_automation_start (i->a);
	
        /* Free stuff to exit */

	if (i->url)
		free (i->url);
	if (i->archive_file_name)
		free (i->archive_file_name);
	free (i);
}



double
scheduler_schedule_next_event (scheduler *s)
{
	PlaylistTemplate *t;
	PlaylistEvent *e, *anchor;
	template_stack_entry *stack_entry;
	AutomationEvent *ae;
	list *categories = NULL;
	Recording *r;
	FileInfo *info;
	double rv;
	url_manager_info *i;
	pthread_t url_manager_thread;
	char channel_name[1024];
	
	pthread_mutex_lock (&(s->mut));
	scheduler_switch_templates (s);
	if (s->template_stack)
		stack_entry = (template_stack_entry *) s->template_stack->data;
	else {
		pthread_mutex_unlock (&(s->mut));
		return s->last_event_end_time;
	}
	
	e = list_get_item (stack_entry->t->events, stack_entry->event_number-1);
	anchor = list_get_item (stack_entry->t->events, e->anchor_event_number-1);
  
	/* compute start time */

	if (anchor && anchor->start_time != -1.0)
		e->start_time = (e->anchor_position) 
			? anchor->end_time
			: anchor->start_time;
	else if (anchor)
		e->start_time = e->end_time = s->last_event_end_time;
	else
		e->start_time = (e->anchor_position)
			? s->prev_event_end_time
			: s->prev_event_start_time;

	e->start_time += e->offset;
	e->end_time = s->last_event_end_time;
  
	/* Create an automation event */

	ae = automation_event_new ();

	/* Try to create a unique channel name */

	sprintf (channel_name, "%s %d %d%lf", e->channel_name, e->template_id, e->number, s->last_event_end_time);
	
	ae->channel_name = strdup (channel_name);
	ae->delta_time = e->start_time-s->prev_event_start_time;
  
	switch (e->type)
	{
	case EVENT_TYPE_SIMPLE_RANDOM:
	case EVENT_TYPE_RANDOM:

		/* The five details are category names, so make a list of them */

		if (*e->detail1)
			categories = string_list_prepend (NULL, e->detail1);
		if (*e->detail2)
			categories = string_list_prepend (categories, e->detail2);
		if (*e->detail3)
			categories = string_list_prepend (categories, e->detail3);
		if (*e->detail4)
			categories = string_list_prepend (categories, e->detail4);
		if (*e->detail5)
			categories = string_list_prepend (categories, e->detail5);

		if (e->type == EVENT_TYPE_SIMPLE_RANDOM) {
			debug_printf (DEBUG_FLAGS_SCHEDULER, "Scheduling simple random event\n");
			r = recording_picker_select (stack_entry->p, categories, -1);
		}
		else {
			debug_printf (DEBUG_FLAGS_SCHEDULER, "Scheduling random event\n");
			r = recording_picker_select (stack_entry->p, categories, e->start_time);
		}

		/* Free the list of categories */

		list_free (categories);

		/* if the recording selection failed, the event has a zero length since
		 * there is no event, so just return the current time
		 */

		if (!r)
		{
			debug_printf (DEBUG_FLAGS_SCHEDULER, "Recording selection failed\n");
			automation_event_destroy (ae);
			ae = NULL;
			break;
		}
      
		/* Add the channel to the mixer */

		mixer_add_file (s->a->m, ae->channel_name, r->path);

		ae->type = AUTOMATION_EVENT_TYPE_ENABLE_CHANNEL;
		ae->level = e->level;
		if (e->start_time-r->audio_in >= stack_entry->t->start_time) {
			e->start_time -= r->audio_in;
			ae->delta_time -= r->audio_in;
		}
		ae->length = r->audio_out;
		e->end_time = e->start_time+r->audio_out;
		recording_free (r);
		break;
	case EVENT_TYPE_FADE:
    
		debug_printf (DEBUG_FLAGS_SCHEDULER, "Scheduling fade on channel %s\n", e->detail1);
		ae->type = AUTOMATION_EVENT_TYPE_FADE_CHANNEL;
		ae->level = e->level;
		ae->length = atof (e->detail1);
		e->end_time = e->start_time + ae->length;
		ae->detail1 = strdup (e->detail1);
		break;

	case EVENT_TYPE_PATH:

		debug_printf (DEBUG_FLAGS_SCHEDULER, "Scheduling path event %s\n", e->detail1);
		info = file_info_new (e->detail1, 1000, 2000);


		/* Add channel to the mixer */

		mixer_add_file (s->a->m, ae->channel_name, e->detail1);

		ae->type = AUTOMATION_EVENT_TYPE_ENABLE_CHANNEL;
		ae->detail1 = strdup (e->detail1);
		ae->level = e->level;
		e->start_time -= info->audio_in;
		ae->delta_time -= info->audio_in;
		ae->length = info->audio_out;
		e->end_time = e->start_time+info->audio_out;
		file_info_free (info);
		break;
	case EVENT_TYPE_URL:

			i = malloc (sizeof(url_manager_info));
			i->url = strdup (e->detail1);
			i->archive_file_name = strdup (e->detail3);
			i->start_time = e->start_time;
			i->end_time = i->start_time+atof (e->detail2);
			if (i->end_time > stack_entry->t->end_time)
				i->end_time = stack_entry->t->end_time;
			e->start_time = i->start_time;
			e->end_time = i->end_time;
			i->retry_delay = 1;
			i->end_fade = stack_entry->t->end_prefade;
			i->m = s->a->m;
			i->a = s->a;
			pthread_create (&url_manager_thread, NULL, url_manager, (void *) i);
		
			/* Switch to fallback template and schedule backup program */

			if (stack_entry->t->fallback_id == -1) {
				s->prev_event_start_time = e->start_time;
				s->last_event_end_time = s->prev_event_end_time = i->end_time;
				
                                /* Schedule end fade */

				if (i->end_fade > 0) {
					ae->type = AUTOMATION_EVENT_TYPE_FADE_ALL;
					ae->delta_time = i->end_time-i->start_time-i->end_fade;
					ae->length = i->end_fade;
					ae->level = 0;
					mixer_automation_add_event (s->a, ae);
				}
				ae = NULL;
				break;
			}
			t = get_playlist_template_by_id (s->db, stack_entry->t->fallback_id);
			t->type = TEMPLATE_TYPE_FALLBACK;

			/* Reset scheduling stuff */

			s->prev_event_start_time = s->last_event_end_time = s->prev_event_end_time = e->start_time;
			stack_entry->event_number++;
			if (stack_entry->event_number > stack_entry->length && !stack_entry->t->repeat_events) {
				t->start_time = stack_entry->t->start_time;
				t->end_time = stack_entry->t->end_time;
				scheduler_pop_template (s);
			}
			else {
				if (stack_entry->event_number > stack_entry->length)
					stack_entry->event_number = 1;
				t->start_time = i->start_time+i->retry_delay;
				t->end_time = i->end_time;
			}
			t->end_prefade = stack_entry->t->end_prefade;
			t->fallback_id = -1;
			scheduler_push_template (s, t, 1);
			pthread_mutex_unlock (&(s->mut));
			return scheduler_schedule_next_event (s);
			break;
	}
  
	/* If the end time of the event falls outside this template, switch to the fallback */

	if (e->end_time > stack_entry->t->end_time && stack_entry->t->fallback_id != -1 && stack_entry->t->repeat_events) {
		if (ae)
			automation_event_destroy (ae);
		t = get_playlist_template_by_id (s->db, stack_entry->t->fallback_id);
		t->type = TEMPLATE_TYPE_FALLBACK;
		t->fallback_id = -1;
		t->end_prefade = stack_entry->t->end_prefade;
		t->start_time = stack_entry->t->start_time;
		t->end_time = stack_entry->t->end_time;
		scheduler_pop_template (s);
		debug_printf (DEBUG_FLAGS_SCHEDULER, "Recording wouldn't fit, switching to fallback templae\n");
		scheduler_push_template (s, t, 1);
		pthread_mutex_unlock (&(s->mut));
		return scheduler_schedule_next_event (s);
	}
	    		
	if (ae) {
		
		mixer_automation_add_event (s->a, ae);
		s->prev_event_start_time = e->start_time;
		s->prev_event_end_time = e->end_time;
		if (e->end_time > s->last_event_end_time)
			s->last_event_end_time = e->end_time;
	}  
	else {
		s->prev_event_end_time = s->last_event_end_time = s->last_event_end_time;
		e->start_time = e->end_time = -1;
	}
	if (e->end_time < stack_entry->t->end_time) {

		/* More stuff to schedule */

		stack_entry->event_number++;
		if (stack_entry->event_number > stack_entry->length) {
			if (stack_entry->t->repeat_events)
				stack_entry->event_number = 1;
			else {
				if (stack_entry->t->fallback_id == -1) {
					s->prev_event_end_time = s->last_event_end_time = stack_entry->t->end_time;
					t = NULL;
				}
				else {
					t = get_playlist_template_by_id (s->db, stack_entry->t->fallback_id);
					t->type = TEMPLATE_TYPE_FALLBACK;
					t->start_time = stack_entry->t->start_time;
					t->end_time = stack_entry->t->end_time;
					t->fallback_id = -1;
					t->end_prefade = stack_entry->t->end_prefade;
				}
				scheduler_pop_template (s);
				if (t)
					scheduler_push_template (s, t, 1);
			}
		}
	}
	rv = s->prev_event_start_time;
	pthread_mutex_unlock (&(s->mut));
	return rv;
}



void *
scheduler_main_thread (void *data)
{
	scheduler *s = (scheduler *) data;
	double target, current;
  
	pthread_mutex_lock (&(s->mut));
	current = target = s->prev_event_end_time;
	pthread_mutex_unlock (&(s->mut));
	debug_printf (DEBUG_FLAGS_SCHEDULER, "Scheduler main thread started\n");
	while (1) {
		pthread_mutex_lock (&(s->mut));
		if (!s->running) {
			pthread_mutex_unlock (&(s->mut));
			break;
		}
		target += s->preschedule;
		pthread_mutex_unlock (&(s->mut));
		while (current < target) {
			current = scheduler_schedule_next_event (s);
		}
		debug_printf (DEBUG_FLAGS_SCHEDULER, "Scheduler sleeping for %lf seconds\n", current-target-s->preschedule);
		usleep ((current-target-s->preschedule)*1000000);
		target = current;
	}
}



void
scheduler_start (scheduler *s,
		 double preschedule)
{
	if (!s)
		return;
	pthread_mutex_lock (&(s->mut));
	if (s->running)
	{
		debug_printf (DEBUG_FLAGS_SCHEDULER, "Starting scheduler\n");
		pthread_mutex_unlock (&(s->mut));
		return;
	}
	s->preschedule = preschedule;
	s->running = 1;
	pthread_create (&(s->scheduler_thread), NULL, scheduler_main_thread, (void *) s);
	pthread_mutex_unlock (&(s->mut));
}



