#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <malloc.h>
#include "mixer.h"
#include "mixerevent.h"
#include "mixerchannel.h"
#include "vorbismixerchannel.h"
#include "mixeroutput.h"
#include "mixerevent.h"



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




static void
mixer_do_events (mixer *m)
{
  MixerEvent *e;

  if (!m)
    return;
  if (!m->events)
    return;
  e = (MixerEvent *) m->events->data;
  if (!e)
    return;
  if (e->start_time <= m->cur_time)
    {
      MixerChannel *ch;
      
      /* Do event */

      switch (e->type)
	{
	case MIXER_EVENT_TYPE_ADD_CHANNEL:
	    ch = vorbis_mixer_channel_new (e->channel_name, e->detail1);
	    ch->level = e->level;

	    /* This looks backwards, since thie internal API is expected to be
	     * called during a mutex lock, we should unlock it before calling
	     * any external mixer API.
	     */
	    
	    mixer_unlock (m);
	    mixer_add_channel (m, ch);
	    mixer_patch_channel_all (m, e->channel_name);
	    mixer_lock (m);
	    break;
	case MIXER_EVENT_TYPE_FADE_CHANNEL:
	  mixer_unlock (m);
	  mixer_fade_channel (m, e->channel_name, e->level, atof (e->detail1));
	  mixer_lock (m);
	  break;
	case MIXER_EVENT_TYPE_FADE_ALL:
	mixer_unlock (m);
	mixer_fade_all (m, e->level, e->end_time-e->start_time);
	mixer_lock (m);
	break;
	case MIXER_EVENT_TYPE_DELETE_ALL:
	  mixer_unlock (m);
	  mixer_delete_all_channels (m);
	  mixer_lock (m);
	  break;
	default:
	  fprintf (stderr, "Bad mixer event.\n");
	  abort ();
	}
      mixer_event_free (e);
      m->events = list_delete_item (m->events, m->events);
    }
  return;
}



static void *
mixer_main_thread (void *data)
{
  mixer *m = (mixer *) data;
  list *tmp, *tmp2;
  short *tmp_buffer;
  int tmp_buffer_size, tmp_buffer_length;
  
  if (!m)
    {
      return NULL;
    }

  tmp_buffer_size = 48000*2*sizeof(short)*MIXER_LATENCY;
  tmp_buffer = malloc (tmp_buffer_size);
  
  while (1)
    {
      mixer_lock (m);
      if (!m->running)
	{
	  mixer_unlock (m);
	  break;
	}
      if (m->cur_time > 86400)
	m->cur_time -= 86400;
      mixer_do_events (m);
      if (!m->outputs)
	{
	  m->cur_time += MIXER_LATENCY;
	  mixer_unlock (m);
	  usleep (MIXER_LATENCY*1000000);
	  continue;
	}
      for (tmp = m->outputs; tmp; tmp = tmp->next)
	{
	  MixerOutput *o = (MixerOutput *) tmp->data;

	  mixer_output_reset_output (o);
	}
      tmp = m->channels;
      while (tmp)
	{
	  MixerChannel *ch = (MixerChannel *) tmp->data;
	  list *next = tmp->next;
	  
	  /* If this channel is disabled, skip it */

	  if (!ch->enabled)
	    {
	      tmp = next;
	      continue;
	    }
	  
	  /* If this channel has no more data, kill it */

	  if (ch->data_end_reached)
	    {
	    
	      /* Get rid of this channel */

	      m->channels = list_delete_item (m->channels, tmp);
	      mixer_channel_destroy (ch);
	      tmp = next;
	      continue;
	    }
	  tmp_buffer_length = mixer_channel_get_data (ch,
						      tmp_buffer,
						      ch->rate*ch->channels*MIXER_LATENCY);
	for (tmp2 = ch->outputs; tmp2; tmp2 = tmp2->next)
	  {
	    MixerOutput *o = (MixerOutput *) tmp2->data;
	    mixer_output_add_output (o, tmp_buffer, tmp_buffer_length);
	  }
	tmp = next;
	}
    for (tmp = m->outputs; tmp; tmp = tmp->next)
      {
	MixerOutput *o = (MixerOutput *) tmp->data;
	mixer_output_post_output (o);
      }
    m->cur_time += MIXER_LATENCY;
    mixer_unlock (m);
    }
  free (tmp_buffer);
}



mixer *
mixer_new (void)
{
  mixer *m;

  m = malloc (sizeof(mixer));
  if (!m)
    return NULL;

  /* Setup mutex to protect mixer data */

  pthread_mutex_init (&(m->mutex), NULL);

  m->cur_time = 0.0;
  m->channels = m->outputs = m->events = NULL;

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
  mixer_lock (m);
  m->thread = 0;
  mixer_unlock (m);
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
  
  /* Free channel list */

  for (tmp = m->channels; tmp; tmp = tmp->next)
    mixer_channel_destroy ((MixerChannel *) tmp->data);
  list_free (m->channels);

  /* Free output list */

  for (tmp = m->outputs; tmp; tmp = tmp->next)
    mixer_output_destroy ((MixerOutput *) tmp->data);
  list_free (m->outputs);

  mixer_unlock (m);
  free (m);
}



void
mixer_add_channel (mixer *m,
		   MixerChannel *ch)
{
  if (!m)
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
		     const char *output_name)
{
  MixerChannel *ch;
  MixerOutput *o;

  if (!m)
    return;
  ch = mixer_get_channel (m, channel_name);
  o = mixer_get_output (m, output_name);

  if (!ch || !o)
    return;

  mixer_lock (m);
  ch->outputs = list_prepend (ch->outputs, o);
  mixer_unlock (m);
}



void
mixer_patch_channel_all (mixer *m,
			 const char *channel_name)
{
  MixerChannel *ch;

  if (!m)
    return;
  ch = mixer_get_channel (m, channel_name);

  if (!ch)
    return;
  
  mixer_lock (m);
  if (ch->outputs)
    list_free (ch->outputs);
  ch->outputs = list_copy (m->outputs);
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
  time_t cur_time;
  struct tm *tp;
  double mixer_time;
  
  if (!m)
    return;

  cur_time = time (NULL);
  tp = localtime (&cur_time);

  mixer_time = tp->tm_hour*3600+tp->tm_min*60+tp->tm_sec;
  mixer_lock (m);
  m->cur_time = mixer_time;
  mixer_unlock (m);
}



void
mixer_insert_event (mixer *m,
		    MixerEvent *new_event)
{
  list *tmp;
  int running;
  
  if (!m)
   return;

  mixer_lock (m);
  for (tmp = m->events; tmp; tmp = tmp->next)
   {
     MixerEvent *e = (MixerEvent *) tmp->data;

     if (new_event->start_time < e->start_time)
       break;
   }
  if (tmp)
    {
      list *new_item = list_insert_before (tmp, new_event);
      if (tmp == m->events)
	m->events = new_item;
    }
  else
    m->events = list_append (m->events, new_event);
  running = m->running;
  mixer_unlock (m);
  if (!running)
    mixer_start (m);
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
