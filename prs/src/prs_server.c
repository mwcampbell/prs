#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include "db.h"
#include "mixer.h"
#include "ossmixeroutput.h"
#include "shoutmixeroutput.h"
#include <shout/shout.h>
#include "vorbismixerchannel.h"



static mixer *global_mixer = NULL;



static void
prs_signal_handler (int signum)
{
  switch (signum)
    {
    case SIGUSR1:
      fprintf (stderr, "User signal 1 trapped.\n");
      setup_streams (global_mixer);
      break;
    }
}



static shout_conn_t *
get_shout_connection (const char * stream,
		      shout_conn_t *orig)
{
  char key[1024];
  char *value;
  shout_conn_t *c = malloc (sizeof (shout_conn_t));
  if (orig)
    memcpy (c, orig, sizeof (shout_conn_t));
  else
    memset (c, 0, sizeof (shout_conn_t));
  
  sprintf (key, "%s_ip", stream);
  value = get_config_value (key);
  if (value)
    c->ip = value;
  sprintf (key, "%s_port", stream);
  value = get_config_value (key);
  if (value)
    c->port = atoi (value);
  free (value);


  /* I f we don't have  a valid ip and port, stop now */

  if (!c->ip || c->port <= 0)
    {
      free (c);
      return NULL;
    }

  sprintf (key, "%s_mount", stream);
  value = get_config_value (key);
  if (value)
    c->mount = value;
  sprintf (key, "%s_password", stream);
  value = get_config_value (key);
  if (value)
    c->password = value;
  sprintf (key, "%s_aim", stream);
  value = get_config_value (key);
  if (value)
    c->aim = value;
  sprintf (key, "%s_icq", stream);
  value = get_config_value (key);
  if (value)
    c->icq = value;
  sprintf (key, "%s_irc", stream);
  value = get_config_value (key);
  if (value)
    c->irc = value;
  sprintf (key, "%s_name", stream);
  value = get_config_value (key);
  if (value)
    c->name = value;
  sprintf (key, "%s_url", stream);
  value = get_config_value (key);
  if (value)
    c->url = value;
  sprintf (key, "%s_genre", stream);
  value = get_config_value (key);
  if (value)
    c->genre = value;
  sprintf (key, "%s_description", stream);
  value = get_config_value (key);
  if (value)
    c->description = value;
  sprintf (key, "%s_bitrate", stream);
  value = get_config_value (key);
  if (value)
    c->bitrate = atoi (value);
  free (value);
  sprintf (key, "%s_ispublic", stream);
  value = get_config_value (key);
  if (value)
    c->ispublic = atoi (value);
  free (value);
  return c;
}



static void
setup_streams (mixer *m)
{
  int i;
  char stream_name[1024];
  MixerOutput *o;
  shout_conn_t *original, *new;
  
  for (i = 1; i <= 5; i++)
    {
      sprintf (stream_name, "stream%d", i);
      o = mixer_get_output (m, stream_name);
      if (o)
	original = shout_mixer_output_get_connection (o);
      else
	original = NULL;
      new = get_shout_connection (stream_name, original);    
      if (o)
	shout_mixer_output_set_connection (o, new);
    else
      {
	if (new)
	  {
	    o = shout_mixer_output_new (stream_name, 44100, 2, new);
	    mixer_add_output (m, o);
	  }
      }
    }
}




static double
process_playlist_event (PlaylistTemplate *t,
			int event_number,
			RecordingPicker *p,
			mixer *m)
{

  /* Keep track of last event start and end times */

  static double last_start_time = -1.0, last_end_time = -1.0;

  PlaylistEvent *e;
  PlaylistEvent *anchor_event;
  Recording *r;
  list *categories;
  MixerEvent *me;
  
  /* Ensure that last_start_time and last_end_time are initialized */
  
  if (last_start_time == -1.0)
    last_start_time = last_end_time = mixer_get_time (m);

  /* Get the PlaylistEvent from the template */

  e = playlist_template_get_event (t, event_number);
  if (!e)
    return last_start_time;

  /* Get the event to which this event is anchored */

  if (e->anchor_event_number >= 0)
    anchor_event = playlist_template_get_event (t, e->anchor_event_number);
  else
    anchor_event = NULL;
  
  /* Create the mixer event */

  me = (MixerEvent *) malloc (sizeof (MixerEvent));
  memset (me, 0, sizeof(MixerEvent));
  
  /* Compute the start time of this event */

  if (anchor_event && anchor_event->start_time != -1.0)
    me->start_time = last_start_time = 
    (e->anchor_position)
      ? (anchor_event->end_time+e->offset)
      : (anchor_event->start_time+e->offset);
  else
    me->start_time =
      (e->anchor_position)
      ? (last_end_time+e->offset)
      : (last_start_time+e->offset);
  
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
      me->level = e->level;
      me->start_time -= r->audio_in;
      me->end_time = me->start_time+r->audio_out;
      recording_free (r);
      break;
    case EVENT_TYPE_FADE:
    
      me->type = MIXER_EVENT_TYPE_FADE_CHANNEL;
      me->level = e->level;
      me->end_time = me->start_time + atof (e->detail1);
      me->detail1 = strdup (e->detail1);
      break;

    case EVENT_TYPE_PATH:

      r = find_recording_by_path (e->detail1);
      me->type = MIXER_EVENT_TYPE_ADD_CHANNEL;
      me->detail1 = strdup (r->path);
      me->level = e->level;
      me->start_time -= r->audio_in;
      me->end_time = me->start_time+r->audio_out;
      recording_free (r);
      break;
    }
  
  if (me)
    {

      /* Wrap around at midnight */

      if (me->end_time > 86400)
	me->end_time -= 86400;
      e->start_time = last_start_time = me->start_time;
      e->end_time = last_end_time = me->end_time;
      mixer_insert_event (m, me);
      return me->start_time;
    }
  else
    return last_start_time;
}



static double
execute_playlist_template (PlaylistTemplate *t,
			   mixer *m,
			   double cur_time)
{
  int length;
  int i;
  RecordingPicker *p;
	        
  if (!t)
    return cur_time;

  p = recording_picker_new (t->artist_exclude, t->recording_exclude);
  do
    {
      length = list_length (t->events);
      for (i = 1; i <= length; i++)
	{
	  double start_time = process_playlist_event (t,
						      i,
						      p,
						      m);
	    
	  /* If we just queued an event more than 10 seconds in the future,
	   * wait the difference less ten seconds
	   */

	  if (start_time > cur_time+10)
	    {
	      usleep ((unsigned long)(start_time-cur_time-10)*1000000);
	      cur_time = mixer_get_time (m)-10;
	    }

	  if (cur_time > t->end_time || cur_time < t->start_time)
	    break;
	}
    }
  while (t->repeat_events && cur_time > t->start_time && cur_time < t->end_time);
  recording_picker_free (p);
  return cur_time;
}



static void *
playlist_main_thread (void *data)
{
  mixer *m = (mixer *) data;
  double cur_time;
  PlaylistTemplate *t;

  cur_time = mixer_get_time (m);
  
  while (1)
    {
      double new_time;

      t = get_playlist_template (cur_time);
      if (t)
	{
	  new_time = execute_playlist_template (t, m, cur_time);
playlist_template_free (t);
	}
      else
      {

	/* Wait ten seconds, then re-sync wih the mixer and try again */

	usleep (10000000);
	cur_time = mixer_get_time (m);
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
  
  signal (SIGUSR1, prs_signal_handler);
  connect_to_database ("prs");
  global_mixer = m = mixer_new ();
  setup_streams (m);
  o = oss_mixer_output_new ("soundcard",
			    44100,
			    2);
  mixer_add_output (m, o);
  mixer_sync_time (m);

  printf ("Running as pid %d.\n", getpid ());
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



