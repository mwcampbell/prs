#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <signal.h>
#include "scheduler.h"
#include "db.h"



typedef struct {
	PlaylistTemplate *t;
	int event_number;
	int length;
	RecordingPicker *p;
} template_stack_entry;



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
}



static void
scheduler_pop_template (scheduler *s)
{
	template_stack_entry *e;

	if (!s)
		return;
	if (!s->template_stack)
		return;
	e = (template_stack_entry *) s->template_stack->data;
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
	pthread_kill (thread, SIGKILL);
	list_free (s->template_stack);
	free (s);
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
	double rv;
  
	if (!s)
		return;
	pthread_mutex_lock (&(s->mut));
	if (!s->template_stack) {

		/* Get the current template */

		t = get_playlist_template (s->db, s->last_event_end_time);
		if (!t) {
			pthread_mutex_unlock (&(s->mut));
			return s->prev_event_end_time;
		}
		scheduler_push_template (s, t, 1);
	}

	stack_entry = (template_stack_entry *) s->template_stack->data;
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
	ae->channel_name = strdup (e->channel_name);
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

		if (e->type == EVENT_TYPE_SIMPLE_RANDOM)
			r = recording_picker_select (stack_entry->p, categories, -1);
		else
			r = recording_picker_select (stack_entry->p, categories, e->start_time);

		/* Free the list of categories */

		list_free (categories);

		/* if the recording selection failed, the event has a zero length since
		 * there is no event, so just return the current time
		 */

		if (!r)
		{
			automation_event_destroy (ae);
			ae = NULL;
			break;
		}
      
		ae->type = AUTOMATION_EVENT_TYPE_ADD_CHANNEL;
		ae->detail1 = strdup (r->path);
		ae->level = e->level;
		e->start_time -= r->audio_in;
		ae->delta_time -= r->audio_in;
		ae->length = r->audio_out;
		e->end_time = e->start_time+r->audio_out;
		recording_free (r);
		break;
	case EVENT_TYPE_FADE:
    
		ae->type = AUTOMATION_EVENT_TYPE_FADE_CHANNEL;
		ae->level = e->level;
		ae->length = atof (e->detail1);
		e->end_time = e->start_time + ae->length;
		ae->detail1 = strdup (e->detail1);
		break;

	case EVENT_TYPE_PATH:

		r = find_recording_by_path (s->db, e->detail1);
		ae->type = AUTOMATION_EVENT_TYPE_ADD_CHANNEL;
		ae->detail1 = strdup (r->path);
		ae->level = e->level;
		e->start_time -= r->audio_in;
		ae->delta_time -= r->audio_in;
		ae->length = r->audio_out;
		e->end_time = e->start_time+r->audio_out;
		recording_free (r);
		break;
	}
  
	/* If the start time of the event falls outside this template, go to a new one */

	if (e->start_time > stack_entry->t->end_time)
	{
		if (ae)
			automation_event_destroy (ae);
		scheduler_pop_template (s);
		s->prev_event_end_time = s->last_event_end_time;
		pthread_mutex_unlock (&(s->mut));
		return scheduler_schedule_next_event (s);
	}
	if (ae)
	{
		mixer_automation_add_event (s->a, ae);
		s->prev_event_start_time = e->start_time;
		s->prev_event_end_time = e->end_time;
		if (e->end_time > s->last_event_end_time)
			s->last_event_end_time = e->end_time;
	}  
	else
		e->start_time = e->end_time = -1;
	stack_entry->event_number++;
	if (stack_entry->event_number > stack_entry->length)
		stack_entry->event_number = 1;
	rv = s->prev_event_start_time;
	pthread_mutex_unlock (&(s->mut));
	return rv;
}



void *
scheduler_main_thread (void *data)
{
	scheduler *s = (scheduler *) data;
	double target, current;
  
	nice (20);
	pthread_mutex_lock (&(s->mut));
	current = target = s->prev_event_end_time;
	pthread_mutex_unlock (&(s->mut));
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
		usleep ((current-target)*900000);
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
		pthread_mutex_unlock (&(s->mut));
		return;
	}
	s->preschedule = preschedule;
	s->running = 1;
	pthread_create (&(s->scheduler_thread), NULL, scheduler_main_thread, (void *) s);
	pthread_mutex_unlock (&(s->mut));
}
  
