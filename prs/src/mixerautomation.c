#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <signal.h>
#include "mixerautomation.h"
#include "db.h"



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



static void
mixer_automation_log_event (MixerAutomation *a,
			    AutomationEvent *e)
{
  Recording *r;

  switch (e->type)
    {
      case AUTOMATION_EVENT_TYPE_ADD_CHANNEL:
	r = find_recording_by_path (a->db, e->detail1);
	if (r)
	  add_log_entry (a->db, r->id, (int) a->last_event_time,
			 (int) e->length);
	recording_free (r);
	break;
    }
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
  pthread_mutex_init (&(a->mut), NULL);
  return a;
}



void
mixer_automation_destroy (MixerAutomation *a)
{
  list *tmp;
  if (!a)
    return;
  if (a->automation_thread > 0)
    {
      a->running = 0;
      pthread_kill (a->automation_thread, SIGKILL);
    }
  for (tmp = a->events; tmp; tmp = tmp->next)
    {
      automation_event_destroy ((AutomationEvent *) tmp->data);
    }
  free (a);
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
      if (a->running)
	mixer_reset_notification_time (a->m, a->last_event_time+e->delta_time);
  a->events = list_append (a->events, e);
  pthread_mutex_unlock (&(a->mut));
}



void
mixer_automation_next_event (MixerAutomation *a)
{
  AutomationEvent *e;
  MixerChannel *ch;      
  double mixer_time;

  if (!a)
    return;
  pthread_mutex_lock (&(a->mut));
  if (a->events)
    e = (AutomationEvent *) a->events->data;
  else
    {
      pthread_mutex_unlock (&(a->mut));
      return;
    }
    
  /* Do event */

  switch (e->type)
    {
    case AUTOMATION_EVENT_TYPE_ADD_CHANNEL:
      if (strcmp (e->detail1 + strlen (e->detail1) - 4, ".mp3") == 0)
	ch = mp3_mixer_channel_new (e->channel_name, e->detail1,
				    a->m->latency);
      else
	ch = vorbis_mixer_channel_new (e->channel_name, e->detail1,
				       a->m->latency);
      ch->level = e->level;

      mixer_add_channel (a->m, ch);
      mixer_patch_channel_all (a->m, e->channel_name);
      break;
    case AUTOMATION_EVENT_TYPE_FADE_CHANNEL:
      mixer_fade_channel (a->m, e->channel_name, e->level, atof (e->detail1));
      break;
    case AUTOMATION_EVENT_TYPE_FADE_ALL:
      mixer_fade_all (a->m, e->level, e->length);
      break;
    case AUTOMATION_EVENT_TYPE_DELETE_ALL:
      mixer_delete_all_channels (a->m);
      break;
    }
  a->events = list_delete_item (a->events, a->events);
  a->last_event_time = mixer_get_time (a->m);
  mixer_automation_log_event (a, e);
  automation_event_destroy (e);
  pthread_mutex_unlock (&(a->mut));
}



static void *
mixer_automation_main_thread (void *data)
{
  MixerAutomation *a = (MixerAutomation *) data;
  db_thread_init (a->db);

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
      if (!a->running)
	{
	  pthread_mutex_unlock (&(a->mut));
	  break;
	}
      pthread_mutex_unlock (&(a->mut));
      mixer_automation_next_event (a);
    }
  pthread_mutex_lock (&(a->mut));
  a->automation_thread = 0;
  pthread_mutex_unlock (&(a->mut));
  db_thread_end (a->db);
}



int
mixer_automation_start (MixerAutomation *a)
{
  AutomationEvent *e;
  
  if (!a)
    return -1;
  pthread_mutex_lock (&(a->mut));
  if (a->running)
    {
      pthread_mutex_unlock (&(a->mut));
      return -1;
    }
  a->running = 1;
  pthread_create (&(a->automation_thread), NULL, mixer_automation_main_thread, (void *) a);
  if (a->events)
    {
      e = (AutomationEvent *) a->events->data;
      if (a->last_event_time+e->delta_time < mixer_get_time (a->m))
	mixer_reset_notification_time (a->m, a->last_event_time+e->delta_time);
      }
  pthread_mutex_unlock (&(a->mut));
  return 0;
}



int
mixer_automation_stop (MixerAutomation *a)
{
  pthread_t thread;
  
  if (!a)
    return -1;
  pthread_mutex_lock (&(a->mut));
  mixer_reset_notification_time (a->m, mixer_get_time (a->m));
  a->running = 0;
  thread = a->automation_thread;
  pthread_mutex_unlock (&(a->mut));
  pthread_join (thread, NULL);
  return 0;
}



void
mixer_automation_set_start_time (MixerAutomation *a,
				double start_time)
{
  AutomationEvent *e;

  if (!a)
    return;
  pthread_mutex_lock (&(a->mut));
  a->last_event_time = start_time;
  if (a->events)
    e = (AutomationEvent *) a->events->data;
  else
    e = NULL;
  if (e)
    mixer_reset_notification_time (a->m, a->last_event_time+e->delta_time);
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
  fprintf (stderr, "LastEvent: %s", ctime (&lt));
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
      fprintf (stderr, "   %s   %s   %lf", ctime (&st), ctime (&et), e->delta_time);
    }
  pthread_mutex_unlock (&(a->mut));
  return rv;
}
