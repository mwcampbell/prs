#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include "db.h"
#include "mixerautomation.h"
#include "ossmixeroutput.h"
#include "shoutmixeroutput.h"
#include "vorbismixerchannel.h"
#include "ossmixerchannel.h"
#include "global_data.h"
#include "scheduler.h"
#include "audiofilter.h"
#include "audiocompressor.h"






static void
sound_on (mixer *m)
{
  MixerOutput *o;
  
  o = mixer_get_output (m, "soundcard");
  o->enabled = 1;
}



static void
sound_off (mixer *m)
{
  MixerOutput *o;
  
  o = mixer_get_output (m, "soundcard");
  o->enabled = 0;
}



static void
mic_on (mixer *m)
{
  MixerChannel *ch;
  
  ch = oss_mixer_channel_new ("mic", 44100, 2);
  if (!ch)
    {
      fprintf (stderr, "Couldn't turn mic on\n");
      return;
    }
  mixer_fade_all (m, .3, 1.0);
  if (!global_data_get_soundcard_duplex ())
    mixer_delete_output (m, "soundcard");
  mixer_add_channel (m, ch);
  mixer_patch_channel_all (m, "mic");
}



static void
mic_off (mixer *m)
{
  mixer_delete_channel (m, "mic");
  if (!global_data_get_soundcard_duplex ())
    {
      MixerOutput *o = oss_mixer_output_new ("soundcard", 44100, 2);
      mixer_add_output (m, o);
    }
  mixer_fade_all (m, 1.0, 1.0);
}



static void
prs_signal_handler (int signum)
{
  switch (signum)
    {
    case SIGUSR1:
      global_data_set_flag (PRS_FLAG_STREAM_RESET_REQUEST);
      break;
    }
}



static shout_conn_t *
get_shout_connection (const char * stream)
{
  char key[1024];
  char *value;
  shout_conn_t *c = malloc (sizeof (shout_conn_t));
  shout_init_connection (c);  
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
  shout_conn_t *new;
  
  for (i = 1; i <= 5; i++)
    {
      sprintf (stream_name, "stream%d", i);
      new = get_shout_connection (stream_name);    
      o = mixer_get_output (m, stream_name);
      if (o && new)
	shout_mixer_output_set_connection (o, new);
      else if (new)
	{
	  o = shout_mixer_output_new (stream_name, 44100, 2, new);
	  mixer_add_output (m, o);
	}
    }
}



int main (void)
{
  mixer *m;
  MixerOutput *o;
  MixerChannel *ch;
  pthread_t playlist_thread;
  char input[81];
  int done = 0;
  MixerAutomation *a;
  scheduler *s;
  AudioFilter *f;
  
  signal (SIGUSR1, prs_signal_handler);
  m = mixer_new ();
  mixer_sync_time (m);
  setup_streams (m);
  o = oss_mixer_output_new ("soundcard",
			    44100,
			    2);
  f = audio_compressor_new (44100, 2, 44100*2*MIXER_LATENCY);
  audio_compressor_add_band (f,
			     50,
			     -30,
			     3,
			     .01,
			     5,
			     4);
  audio_compressor_add_band (f,
			     100,
			     -30,
			     3,
			     .01,
			     5,
			     4);
  audio_compressor_add_band (f,
			     4000,
			     -30,
			     3,
			     .01,
			     5,
			     5);
  audio_compressor_add_band (f,
			     9000,
			     -25,
			     3,
			     .01,
			     5,
			     5);
  audio_compressor_add_band (f,
			     15000,
			     -5,
			     10,
			     .01,
			     5,
			     4);
  mixer_output_add_filter (o, f);
  mixer_add_output (m, o);
  
  
			   
#if 0
  ch = vorbis_mixer_channel_new ("test", "test.ogg");
  mixer_add_channel (m, ch);
  mixer_patch_channel_all (m, "test");
#endif
  printf ("Running as pid %d.\n", getpid ());
  a = mixer_automation_new (m);
  mixer_start (m);

  s = scheduler_new (a, mixer_get_time (m)+10);

  scheduler_start (s, 10);

  while (!done)
    {
      fprintf (stderr, "PRS$ ");
      fgets (input, 80, stdin);
      input[strlen(input)-1] = 0;
      if (!strcmp(input, "quit"))
	break;
      if (!strcmp (input, "on"))
	mic_on (m);
      if (!strcmp (input, "off"))
	mic_off (m);
      if (!strcmp (input, "soundon"))
	sound_on (m);
      if (!strcmp (input, "soundoff"))
	sound_off (m);
      if (!strcmp (input, "n"))
	mixer_automation_next_event (a);
      if (!strcmp (input, "start"))
	mixer_automation_start (a);
      if (!strcmp (input, "stop"))
	mixer_automation_stop (a);
      if (!strcmp (input, "date"))
	{
	  time_t t = (time_t) mixer_get_time (m);
	  fprintf (stderr, "%s", ctime (&t));
	}      
    }
  fprintf (stderr, "Destroying scheduler...\r");
  scheduler_destroy (s);
  fprintf (stderr, "Destroying MixerAutomation...\r");
  mixer_automation_destroy (a);
  fprintf (stderr, "Destroying mixer...\r");
  mixer_destroy (m);
  return 0;
}



