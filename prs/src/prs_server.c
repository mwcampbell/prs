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
			RecordingPicker *p,
			mixer *m,
			double cur_time)
{
  Recording *r;
  list *categories;
  MixerEvent *me;
  double rv;
  
  switch (e->type)
    {
    case EVENT_TYPE_SIMPLE_RANDOM:
    case EVENT_TYPE_RANDOM:
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
	r = recording_picker_select (p, categories, cur_time);

      /* if the recording selection failed, return -1 */

      if (!r)
	{
	  rv = -1.0;
	  break;
	  }
      
      /* Create the mixer event */

      me = (MixerEvent *) malloc (sizeof (MixerEvent));
      me->type = MIXER_EVENT_TYPE_ADD_CHANNEL;
      me->detail1 = strdup (r->name);
      me->detail2 = strdup (r->path);
      me->time = cur_time-r->audio_in;
      mixer_insert_event (m, me);
      rv = r->audio_out-r->audio_in;
      recording_free (r);
      break;
    }
  return rv;
}



static void *
playlist_main_thread (void *data)
{
  mixer *m = (mixer *) data;
  double mixer_time = mixer_get_time (m);
  
  while (1)
    {
      PlaylistTemplate *t = get_playlist_template (mixer_time);
      list *events, *tmp;
      RecordingPicker *p;
	        
      if (!t)
	{
	  break;
	}
      events = get_playlist_events_from_template (t);
      p = recording_picker_new (t->artist_exclude, t->recording_exclude);
      do
	{
	  for (tmp = events; tmp; tmp = tmp->next)
	    {
	      double time_to_wait;


	      time_to_wait = process_playlist_event ((PlaylistEvent *) tmp->data,
						     p, m, mixer_time);	      
	      if (time_to_wait > 10)
		usleep ((int)(time_to_wait-10)*1000000);
	      if (time_to_wait >= 0)
		{
		  mixer_time += time_to_wait;
		  if (mixer_time > 86400)
		    mixer_time -= 86400;
		}
	      if (mixer_time > t->end_time || mixer_time < t->start_time)
		break;
	    }
	  }
      while (t->repeat_events && mixer_time > t->start_time && mixer_time < t->end_time);
      playlist_template_free (t);
      playlist_event_list_free (events);
      recording_picker_free (p);
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



