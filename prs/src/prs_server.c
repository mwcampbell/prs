#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include "prs_config.h"
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
#include "multibandaudiocompressor.h"






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
  
  ch = oss_mixer_channel_new ("mic", 44100, 2, m->latency);
  if (!ch)
    {
      fprintf (stderr, "Couldn't turn mic on\n");
      return;
    }
  mixer_fade_all  (m, .2, 1.0);
  if (!global_data_get_soundcard_duplex ())
    mixer_delete_output (m, "soundcard");
  ch->level = .8;
  mixer_add_channel (m, ch);
  mixer_patch_channel_all (m, "mic");
}



static void
mic_off (mixer *m)
{
  mixer_delete_channel (m, "mic");
  if (!global_data_get_soundcard_duplex ())
    {
      MixerOutput *o = oss_mixer_output_new ("soundcard", 44100, 2, m->latency);
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



int main (void)
{
  mixer *m;
  MixerOutput *o;
  MixerChannel *ch;
  MixerBus *b;
  AudioFilter *f;
  pthread_t playlist_thread;
  char input[81];
  int done = 0;
  MixerAutomation *a;
  scheduler *s;
  
  signal (SIGUSR1, prs_signal_handler);
  m = mixer_new (2048);
  mixer_sync_time (m);

  /* Parse configuration file */

  prs_config (m);


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



