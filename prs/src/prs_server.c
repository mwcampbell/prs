#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "db.h"
#include "mixer.h"
#include "ossmixeroutput.h"
#include "shoutmixeroutput.h"
#include "vorbismixerchannel.h"



static double
process_playlist_event (PlaylistEvent *e,
			PlaylistTemplate *t,
			RecordingPicker *p,
			mixer *m)
{

  /* Keep track of last event start and end times */

  static double last_start_time = -1.0, last_end_time = -1.0;

  Recording *r;
  list *categories;
  MixerEvent *me;
  
  /* Create the mixer event */

  me = (MixerEvent *) malloc (sizeof (MixerEvent));

  /* Compute the start time of this event */

  if (last_start_time = -1.0)
    last_start_time = mixer_get_time (m);

  me->start_time = last_start_time = 
    (e->anchor ? last_end_time : last_start_time)+e->offset;

  /* Set the channel name */
  
  me->channel_name = strdup (e->channel_name);

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
	r = recording_picker_select (p, categories, -1);
      else
	r = recording_picker_select (p, categories, me->start_time);

      /* Free the list of categories */

      list_free (categories);

      /* if the recording selection failed, the event has a zero length since
       * there is no event, so just return the current time
       */

      if (!r)
	{
	  free (me);
	  me = NULL;
	  break;
	}
      
      me->type = MIXER_EVENT_TYPE_ADD_CHANNEL;
      me->detail1 = strdup (r->path);
      me->level = 1.0;

      me->end_time = me->start_time+(r->audio_out-r->audio_in);
      recording_free (r);
      break;
    case EVENT_TYPE_FADE_CHANNEL:
    
      me->type = MIXER_EVENT_TYPE_FADE_CHANNEL;
      me->level = atof (e->detail1);
      me->end_time = me->start_time + atof (e->detail2);
      break;
    }
  
  if (me)
    {

      /* Wrap around at midnight */

      if (me->end_time > 86400)
	me->end_time -= 86400;
      last_end_time = me->end_time;
      mixer_insert_event (m, me);
      return me->start_time;
    }
  else
    return last_start_time;
}



static double
execute_playlist_template (PlaylistTemplate *t,
			   mixer *m,
			   double time)
{
  list *events, *tmp;
  RecordingPicker *p;
	        
  if (!t)
    return;

  events = get_playlist_events_from_template (t);
  p = recording_picker_new (t->artist_exclude, t->recording_exclude);
  do
    {
      for (tmp = events; tmp; tmp = tmp->next)
	{
	  double start_time = process_playlist_event ((PlaylistEvent *) tmp->data,
						      t, p, m);
	    
	  /* If we just queued an event more than 10 seconds in the future,
	   * wait the difference less ten seconds
	   */

	  if (start_time > time+10)
	    {
	      usleep (start_time-time-10);
	      time = mixer_get_time (m);
	    }

	  /* An end_time of -1 means don't worry about the end time, otherwise
	   * if the current time is past the end time of this template, stop
	   */

	  if (time > t->end_time || time < t->start_time)
	    break;
	}
    }
  while (t->repeat_events && time > t->start_time && time < t->end_time);
  playlist_event_list_free (events);
  recording_picker_free (p);
  return time;
}



static void *
playlist_main_thread (void *data)
{
  mixer *m = (mixer *) data;
  double time = mixer_get_time (m);
  PlaylistTemplate *t;
  
  while (1)
    {
      t = get_playlist_template (time);
      if (t)
	time = execute_playlist_template (t, m, time);
    else
      {

	/* Wait ten seconds, then re-sync wih the mixer and try again */

	usleep (10000000);
	time = mixer_get_time (m);
      }
    }
  return 0;
}




int main (void)
{
  mixer *m;
  MixerOutput *o;
  pthread_t playlist_thread;
  char input[81];
  int done = 0;
  
  connect_to_database ("prs");
  m = mixer_new ();
  o = shout_mixer_output_new ("shoutcast",
			      44100,
			      2,
			      "127.0.0.1",
			      8000,
			      "marcplanet");
  mixer_add_output (m, o);
  o = oss_mixer_output_new ("soundcard",
			    44100,
			    2);
  mixer_add_output (m, o);
  mixer_sync_time (m);

  pthread_create (&playlist_thread, NULL, playlist_main_thread, m);

  while (!done)
    {
      fprintf (stderr, "PRS$ ");
      fgets (input, 80, stdin);
      input[strlen(input)-1] = 0;
      if (!strcmp(input, "quit"))
	break;
      printf ("You typed %s.\n", input);
    }
  mixer_destroy (m);
  return 0;
}



