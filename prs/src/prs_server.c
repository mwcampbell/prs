#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include "fileinfo.h"
#include "vorbisfileinfo.h"
#include "mp3fileinfo.h"
#include "prs_config.h"
#include "db.h"
#include "mixerautomation.h"
#include "ossmixeroutput.h"
#include "shoutmixeroutput.h"
#include "vorbismixerchannel.h"
#include "ossmixerchannel.h"
#include "scheduler.h"
#include "audiofilter.h"
#include "audiocompressor.h"
#include "multibandaudiocompressor.h"






static void
load_playlist (MixerAutomation *a)
{
	char pl_name[1025];
	char path[1025];
	AutomationEvent *e;
	FILE *fp;
	double delta = 0.0;
	
	fprintf (stderr, "Enter playlist name: ");
	fgets (pl_name, 1024, stdin);
	pl_name[strlen(pl_name)-1] = 0;
	fp = fopen (pl_name, "rb");

	if (!fp) {
		fprintf (stderr, "File not found.\n");
		return;
	}

	while (!feof (fp)) { 
		Recording *r;
		AutomationEvent *e = automation_event_new ();
	
		fgets (path, 1024, fp);
		path[strlen(path)-1] = 0;

		e->detail1 = strdup (path);
		e->level = 1.0;
		e->type = AUTOMATION_EVENT_TYPE_ADD_CHANNEL;
		r = find_recording_by_path (path);
		if (!r) {
			FileInfo *i;
			char *ext;
			
			ext = path + strlen(path)-4;
			if (!strcmp (ext, ".ogg"))
				i = get_vorbis_file_info (path, 2000, 3000);
			if (!strcmp (ext, ".mp3"))
				i = get_mp3_file_info (path, 2000, 3000);
			if (!i) {
				fclose (fp);
				fprintf (stderr, "File not found in playlist.\n");
				return;
			}
			e->channel_name = strdup ("ACB Radio Test");
			e->delta_time = delta-i->audio_in;
			delta = e->length = i->audio_out;
		}
		else {
			e->channel_name = strdup (r->name);
			e->delta_time = delta-r->audio_in;
			delta = e->length = r->audio_out;
		}
		mixer_automation_add_event (a, e);
	}
}


static void
mic_on (mixer *m)
{
	mixer_enable_channel (m, "soundcard", 1);
}



static void
mic_off (mixer *m)
{
	mixer_enable_channel (m, "soundcard", 0);
}


static void
sound_on (mixer *m)
{
	mixer_enable_output (m, "soundcard", 1);
}


static void
sound_off (mixer *m)
{
	mixer_enable_output (m, "soundcard", 0);
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
  
  m = mixer_new (2048);
  mixer_sync_time (m);

  /* Parse configuration file */

  prs_config (m);
			   a = mixer_automation_new (m);
  mixer_start (m);


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
      if (!strcmp (input, "start"))
	      mixer_automation_start (a);
      if (!strcmp (input, "stop"))
	      mixer_automation_stop (a);
      if (!strcmp (input, "load"))
	      load_playlist (a);
    if (!strcmp (input, "soundon"))
	    sound_on (m);
    if (!strcmp (input, "soundoff"))
	    sound_off (m);
    }
  fprintf (stderr, "Destroying scheduler...\r");
  scheduler_destroy (s);
  fprintf (stderr, "Destroying MixerAutomation...\r");
  mixer_automation_destroy (a);
  fprintf (stderr, "Destroying mixer...\r");
  mixer_destroy (m);
  return 0;
}



