#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "mixerautomation.h"



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



MixerAutomation *
mixer_automation_new (mixer *m)
{
  MixerAutomation *a = (MixerAutomation *) malloc (sizeof (MixerAutomation));
  a->m = m;
  a->events = NULL;
  a->last_event_time = mixer_get_time (m);
  a->automation_thread = 0;
  a->running = 0;
  return a;
}



void
mixer_automation_destroy (MixerAutomation *a)
{
  list *tmp;
  if (!a)
    return;
  for (tmp = a->events; tmp; tmp = tmp->next)
    {
      automation_event_destroy ((AutomationEvent *) tmp->data);
    }
  if (a->automation_thread > 0)
    {
      a->running = 0;
      pthread_join ((a->automation_thread), NULL);
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
  if (!a->events)
      if (a->running)
	mixer_reset_notification_time (a->m, a->last_event_time+e->delta_time);
  a->events = list_append (a->events, e);
}



void
mixer_automation_next_event (MixerAutomation *a)
{
  AutomationEvent *e;
  MixerChannel *ch;      
  
  if (!a)
    return;
  if (a->events)
    e = (AutomationEvent *) a->events->data;
  else
    return;

  /* Do event */

  switch (e->type)
    {
    case AUTOMATION_EVENT_TYPE_ADD_CHANNEL:
      ch = vorbis_mixer_channel_new (e->channel_name, e->detail1);
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
    default:
      abort ();
    }
  automation_event_destroy (e);
  a->events = list_delete_item (a->events, a->events);
  a->last_event_time = mixer_get_time (a->m);
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

      double wait_time = (double) (0x7ffffff);
      AutomationEvent *e;

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
	break;


      /* Wait for the mixer to notify us that the specified time has been reached */

      mixer_wait_for_notification (a->m, wait_time);


      /* If someone turned automation off while we were waiting, bail now */

      if (!a->running)
	break;
      mixer_automation_next_event (a);
    }
  a->automation_thread = 0;
}



int
mixer_automation_start (MixerAutomation *a)
{
  AutomationEvent *e;
  
  if (!a)
    return -1;
  if (a->running)
    return -1;
  a->running = 1;
  pthread_create (&(a->automation_thread), NULL, mixer_automation_main_thread, (void *) a);
  if (a->events)
    {
      e = (AutomationEvent *) a->events->data;
      if (a->last_event_time+e->delta_time < mixer_get_time (a->m))
	mixer_reset_notification_time (a->m, a->last_event_time+e->delta_time);
      }
  return 0;
}



int
mixer_automation_stop (MixerAutomation *a)
{
  if (!a)
    return -1;
  mixer_reset_notification_time (a->m, mixer_get_time (a->m));
  a->running = 0;
  pthread_join ((a->automation_thread), NULL);
  return 0;
}



void
mixer_automation_set_start_time (MixerAutomation *a,
				double start_time)
{
  AutomationEvent *e;

  if (!a)
    return;
  a->last_event_time = start_time;
  if (a->events)
    e = (AutomationEvent *) a->events->data;
  else
    e = NULL;
  if (e)
    mixer_reset_notification_time (a->m, a->last_event_time+e->delta_time);
}



double
mixer_automation_get_last_event_end (MixerAutomation *a)
{
  double event_start_time, event_end_time, rv;
  list *tmp;
  
  if (!a)
    return -1.0;

  rv = event_start_time = event_end_time = a->last_event_time;
  for (tmp = a->events; tmp; tmp = tmp->next)
    {
      AutomationEvent *e = (AutomationEvent *) tmp->data;
      event_start_time += e->delta_time;
      event_end_time = event_start_time + e->length;
      if (event_end_time > rv)
	rv = event_end_time;
    }
  return rv;
}

  
