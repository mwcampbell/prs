#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <malloc.h>
#include "mixer.h"
#include "mixerchannel.h"
#include "vorbismixerchannel.h"
#include "mixeroutput.h"
#include "mixerevent.h"



static MixerChannel *
mixer_find_channel (mixer *m,
		    const char *channel_name)
{
  list *tmp;

  if (!m)
    return NULL;
  for (tmp = m->channels; tmp; tmp = tmp->next)
    {
      MixerChannel *ch;

      ch = (MixerChannel *) tmp->data;
      if (!strcmp (channel_name, ch->name))
	break;
    }
  if (tmp)
    return (MixerChannel *) tmp->data;
  else
    return NULL;
}



static MixerOutput *
mixer_find_output (mixer *m,
		   const char *output_name)
{
  list *tmp;

  for (tmp = m->outputs; tmp; tmp = tmp->next)
    {
      MixerOutput *o;

      o = (MixerOutput *) tmp->data;
      if (!strcmp (output_name, o->name))
	break;
    }
  if (tmp)
    return (MixerOutput *) tmp->data;
  else
    return NULL;
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
  if (e->time <= m->time)
    {
      MixerChannel *ch;
      
      /* Do event */

      switch (e->type)
	{
	  case MIXER_EVENT_TYPE_ADD_CHANNEL:
	    ch = vorbis_mixer_channel_new (e->detail1, e->detail2);
	    pthread_mutex_unlock (&m->mutex);
	    mixer_add_channel (m, ch);
	    mixer_patch_channel_all (m, e->detail1);
	    pthread_mutex_lock (&m->mutex);
	    break;
	}
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
      printf ("Main mixer thread died!\n");
      return NULL;
    }
  tmp_buffer_size = 48000*2*sizeof(short)*MIXER_LATENCY;
  tmp_buffer = malloc (tmp_buffer_size);
  
  while (1)
    {
      pthread_mutex_lock (&m->mutex);
      if (!m->running)
	{
	  printf ("Killing mixer thread.\n");
	  pthread_mutex_unlock (&m->mutex);
	  break;
	}
      if (m->time > 86400)
	m->time -= 86400;
      mixer_do_events (m);
      if (!m->outputs)
	{
	  m->time += MIXER_LATENCY;
	  pthread_mutex_unlock (&m->mutex);
	  usleep (MIXER_LATENCY*1000000);
	  continue;
	}
      for (tmp = m->outputs; tmp; tmp = tmp->next)
	{
	  MixerOutput *o = (MixerOutput *) tmp->data;
	  mixer_output_reset_output (o);
	}
      for (tmp = m->channels; tmp; tmp = tmp->next)
	{
	  MixerChannel *ch = (MixerChannel *) tmp->data;
	  tmp_buffer_length = mixer_channel_get_data (ch,
						      tmp_buffer,
						      ch->rate*ch->channels*MIXER_LATENCY);
	for (tmp2 = ch->outputs; tmp2; tmp2 = tmp2->next)
	  {
	    MixerOutput *o = (MixerOutput *) tmp2->data;
	    mixer_output_add_output (o, tmp_buffer, tmp_buffer_length);
	  }
	}
    for (tmp = m->outputs; tmp; tmp = tmp->next)
      {
      MixerOutput *o = (MixerOutput *) tmp->data;
      mixer_output_post_output (o);
      }
    m->time += MIXER_LATENCY;
    pthread_mutex_unlock (&m->mutex);
    }
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

  m->time = 0.0;
  m->channels = m->outputs = m->events = NULL;

  m->running = 0;
  m->thread = NULL;
  return m;
}



int
mixer_start (mixer *m)
{
  if (!m)
    return -1;
  if (m->running || m->thread)
    return -1;
  
  /* Create mixer main thread */

  m->running = 1;
  if (pthread_create (&(m->thread),
		      NULL,
		      mixer_main_thread,
		      (void *) m))
    return -1;
  m->running = 1;
  return 0;
}



int
mixer_stop (mixer *m)
{
  if (!m)
    return -1;
  if (!m->running)
    return -1;
  pthread_mutex_lock (&m->mutex);
  m->running = 0;
  m->thread = NULL;
  pthread_mutex_unlock (&m->mutex);
  return 0;
}



void
mixer_destroy (mixer *m)
{
  list *tmp;

  if (!m)
    return;
  
  pthread_mutex_lock (&(m->mutex));
  
  /* Free channel list */

  for (tmp = m->channels; tmp; tmp = tmp->next)
    mixer_channel_destroy ((MixerChannel *) tmp->data);
  list_free (m->channels);

  /* Free output list */

  for (tmp = m->outputs; tmp; tmp = tmp->next)
    mixer_output_destroy ((MixerOutput *) tmp->data);
  list_free (m->outputs);

  pthread_mutex_unlock (&(m->mutex));
  free (m);
}



void
mixer_add_channel (mixer *m,
		   MixerChannel *ch)
{
  if (!m)
    return;
  pthread_mutex_lock (&(m->mutex));
  m->channels = list_prepend (m->channels, ch);
  pthread_mutex_unlock (&(m->mutex));
}


void
mixer_add_output (mixer *m,
		  MixerOutput *o)
{
  if (!m)
    return;
  pthread_mutex_lock (&(m->mutex));
  m->outputs = list_prepend (m->outputs, o);
  pthread_mutex_unlock (&(m->mutex));
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
  pthread_mutex_lock (&(m->mutex));
  ch = mixer_find_channel (m, channel_name);
  o = mixer_find_output (m, output_name);

  if (!ch || !o)
    return;

  ch->outputs = list_prepend (ch->outputs, o);
  pthread_mutex_unlock (&(m->mutex));
}



void
mixer_patch_channel_all (mixer *m,
			 const char *channel_name)
{
  MixerChannel *ch;

  if (!m)
    return;
  pthread_mutex_lock (&(m->mutex));
  ch = mixer_find_channel (m, channel_name);

  if (!ch)
    return;

  if (ch->outputs)
    list_free (ch->outputs);
  ch->outputs = list_copy (m->outputs);
  pthread_mutex_unlock (&(m->mutex));
}



double
mixer_get_time (mixer *m)
{
  double rv;

  if (!m)
    return 0.0;
  pthread_mutex_lock (&(m->mutex));
  rv = m->time;
  pthread_mutex_unlock (&(m->mutex));
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
  pthread_mutex_lock (&m->mutex);
  m->time = mixer_time;
  pthread_mutex_unlock (&m->mutex);
}



void
mixer_insert_event (mixer *m,
		    MixerEvent *new_event)
{
  list *tmp;

  if (!m)
   return;

  pthread_mutex_lock (&m->mutex);
  for (tmp = m->events; tmp; tmp = tmp->next)
   {
     MixerEvent *e = (MixerEvent *) tmp->data;
     if (new_event->time < e->time)
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
  if (!m->running)
    mixer_start (m);
  pthread_mutex_unlock (&m->mutex);
}

