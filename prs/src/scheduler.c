#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <signal.h>
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
		list_free (s->template_stack);
	free (s);
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
	
	if (e) {
		AutomationEvent *ae = automation_event_new ();
		double start_delta = e->t->end_time-e->t->end_prefade-s->prev_event_start_time;
		if (e->t->end_prefade > 0) {
			ae->type = AUTOMATION_EVENT_TYPE_FADE_ALL;
			ae->delta_time = start_delta;
			ae->length = e->t->end_prefade;
			ae->level = 0;
			mixer_automation_add_event (s->a, ae);
			start_delta = e->t->end_prefade;
			ae = automation_event_new ();
		}
		ae->type = AUTOMATION_EVENT_TYPE_DELETE_ALL;
		ae->delta_time = start_delta;
		ae->length = 0;
		mixer_automation_add_event (s->a, ae);
		s->last_event_end_time = s->prev_event_start_time = s->prev_event_end_time = e->t->end_time;
		scheduler_pop_template (s);
		if (s->template_stack)
			return;
	}

	t = get_playlist_template (s->db, s->last_event_end_time);

	if (!t && !e) {
		s->last_event_end_time += s->preschedule;
		s->prev_event_start_time = s->prev_event_end_time = s->last_event_end_time;
	}
	if (t && !s->template_stack) {

		/* Set new template */

		if (!e) {
			start_time = mixer_get_time (s->a->m);
			if (t->start_time > start_time)
				start_time = t->start_time;
			s->last_event_end_time = s->prev_event_end_time = start_time;
		}
		else {
			s->last_event_end_time = s->prev_event_end_time = s->prev_event_start_time = t->start_time;
		}
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
	
	pthread_mutex_lock (&(s->mut));
	if (s->template_stack)
		stack_entry = (template_stack_entry *) s->template_stack->data;
	else
		stack_entry = NULL;
	if (!stack_entry || s->prev_event_end_time >= stack_entry->t->end_time) {
		scheduler_switch_templates (s);
		if (s->template_stack)
			stack_entry = (template_stack_entry *) s->template_stack->data;
		else {
			pthread_mutex_unlock (&(s->mut));
			return s->prev_event_end_time;
		}
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

		info = file_info_new (e->detail1, 1000, 2000);
		ae->type = AUTOMATION_EVENT_TYPE_ADD_CHANNEL;
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
					ae = automation_event_new ();
				}
				ae->type = AUTOMATION_EVENT_TYPE_DELETE_ALL;
				if (i->end_fade > 0)
					ae->delta_time = i->end_fade;
				else
					ae->delta_time = i->end_time-i->start_time;
				ae->length = 0;
				mixer_automation_add_event (s->a, ae);
				ae = NULL;
				break;
			}
			t = get_playlist_template_by_id (s->db, stack_entry->t->fallback_id);

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
		t->fallback_id = -1;
		t->end_prefade = stack_entry->t->end_prefade;
		t->start_time = stack_entry->t->start_time;
		t->end_time = stack_entry->t->end_time;
		scheduler_pop_template (s);
		scheduler_push_template (s, t, 1);
		pthread_mutex_unlock (&(s->mut));
		return scheduler_schedule_next_event (s);
	}
	    		
	if (ae) {
		
		/* Ensure end_time is within the template */

		mixer_automation_add_event (s->a, ae);
		s->prev_event_start_time = e->start_time;
		s->prev_event_end_time = e->end_time;
		if (e->end_time > s->last_event_end_time)
			s->last_event_end_time = e->end_time;
	}  
	else {
		s->prev_event_start_time = s->prev_event_end_time = s->last_event_end_time = s->last_event_end_time+s->preschedule;
		e->start_time = e->end_time = -1;
	}
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
		usleep ((current-target+s->preschedule)*900000);
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



