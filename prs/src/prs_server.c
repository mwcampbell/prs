#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include "debug.h"
#include "db.h"
#include "prs.h"
#include "fileinfo.h"
#include "vorbisfileinfo.h"
#include "mp3fileinfo.h"
#include "prs_config.h"
#include "mixerautomation.h"
#include "ossmixerchannel.h"
#include "ossmixeroutput.h"
#include "shoutmixeroutput.h"
#include "vorbismixerchannel.h"
#include "ossmixerchannel.h"
#include "scheduler.h"



static void
load_playlist (MixerAutomation *a, Database *db, FILE *in, FILE *out)
{
	char pl_name[1025];
	char path[1025];
	AutomationEvent *e;
	FILE *fp;
	double delta = 0.0;
	
	fprintf (out, "Enter playlist name: ");
	fgets (pl_name, 1024, in);
	pl_name[strlen(pl_name)-1] = 0;
	fp = fopen (pl_name, "rb");

	if (!fp) {
		fprintf (out, "File not found.\n");
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
		r = find_recording_by_path (db, path);
		if (!r) {
			FileInfo *i;
			
			i = file_info_new (path, 1000, 2000);
			if (!i) {
				fclose (fp);
				fprintf (out, "File not found in playlist.\n");
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


static int
prs_session (PRS *prs, FILE *in, FILE *out)
{
  char input[81];
  char *tmp = NULL;
  int done = 0;

  if (prs->password != NULL)
    {
      fprintf (out, "Password: ");
      if (fgets (input, 80, in) == NULL)
	return 0;
      input[strlen (input) - 1] = 0;
      if ((tmp = strchr (input, '\r')) != NULL)
	*tmp = 0;
      if (strcmp (input, prs->password) != 0)
	{
	  fprintf (out, "Password incorrect\n");
	  return 0;
	}
    }

  while (!done)
    {
      fprintf (out, "PRS$ ");
      if (fgets (input, 80, in) == NULL)
	{
	  done = 1;
	  break;
	}
      input[strlen (input) - 1] = 0;
      if ((tmp = strchr (input, '\r')) != NULL)
	*tmp = 0;
      if (!strcmp (input, "shutdown"))
	return 1;
      if (!strcmp (input, "quit"))
	{
	  done = 1;
	  break;
	}
      if (!strcmp (input, "on"))
	mic_on (prs->mixer);
      if (!strcmp (input, "off"))
	mic_off (prs->mixer);
      if (!strcmp (input, "soundon"))
	sound_on (prs->mixer);
      if (!strcmp (input, "soundoff"))
	sound_off (prs->mixer);
      if (!strcmp (input, "n"))
	mixer_automation_next_event (prs->automation);
      if (!strcmp (input, "start"))
	mixer_automation_start (prs->automation);
      if (!strcmp (input, "stop"))
	mixer_automation_stop (prs->automation);
      if (!strcmp (input, "date"))
	{
	  time_t t = (time_t) mixer_get_time (prs->mixer);
	  fprintf (out, "Mixer time: %s", ctime (&t));
	  t = time (NULL);
	  fprintf (out, "system time: %s", ctime (&t));
	}      
      if (!strcmp (input, "load"))
	load_playlist (prs->automation, prs->db, in, out);
    }

  return 0;
}



int
main (int argc, char *argv[])
{
  struct sockaddr_in sa;
  int sock = -1, new_sock = -1;
  int sa_length = sizeof (sa);
  int reuse_addr = 1;
  const char *config_filename = "prs.conf";
  PRS *prs = prs_new ();

  debug_set_flags (DEBUG_FLAGS_ALL);
  if (argc > 1)
    config_filename = argv[1];
  debug_printf (DEBUG_FLAGS_GENERAL, "Loading config file %s\n", config_filename);
  prs_config (prs, config_filename);

  if (prs->telnet_interface)
    {
      /* Set up the server socket. */
      memset (&sa, 0, sizeof (sa));
      sa.sin_family = AF_INET;
      sa.sin_port = htons (prs->telnet_port);
      sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

      if (sock < 0)
	{
	  perror ("unable to create server socket");
	  return 1;
	}

      if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
		      sizeof (reuse_addr)) < 0)
	{
	  perror ("unable to set up server socket");
	  return 1;
	}

      if (bind (sock, (struct sockaddr *) &sa, sizeof (sa)) < 0)
	{
	  perror ("unable to bind server socket");
	  return 1;
	}

      if (listen (sock, 1) < 0)
	{
	  perror ("unable to set up server socket");
	  return 1;
	}

      switch (fork ())
	{
	case 0:
	  setsid ();
	  close (0);
	  close (1);
	  close (2);
	  open ("/dev/null", O_RDONLY);
	  open ("prs.log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	  open ("prs.err", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	  setvbuf (stdout, NULL, _IONBF, 0);
	  setvbuf (stderr, NULL, _IONBF, 0);
	  debug_printf (DEBUG_FLAGS_TELNET, "Forked child process and set up logging.\n");
	  break;
	case -1:
	  perror ("unable to fork");
	  return 1;
	default:
		debug_printf (DEBUG_FLAGS_TELNET, "Parent server process returning.\n");
		return 0;
	}
    }
  
  debug_printf (DEBUG_FLAGS_GENERAL, "Starting prs instance.\n");
  prs_start (prs);

  if (prs->telnet_interface)
    {
	    debug_printf (DEBUG_FLAGS_TELNET, "Waiting for telnet connection.\n");
	    while ((new_sock = accept (sock, (struct sockaddr *) &sa,
				 &sa_length)) >= 0)
	{
	  FILE *sock_fp = fdopen (new_sock, "r+");
	  debug_printf (DEBUG_FLAGS_TELNET, "Starting prs session.\n");
	  if (prs_session (prs, sock_fp, sock_fp))
	    break;
	  else
	    fclose (sock_fp);
	}
    }
  else
    prs_session (prs, stdin, stdout);

  debug_printf (DEBUG_FLAGS_GENERAL, "Destroying prs instance.\n");
  prs_destroy (prs);
  return 0;
}
