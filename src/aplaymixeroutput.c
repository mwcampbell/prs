/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "debug.h"
#include "aplaymixeroutput.h"
#include "list.h"


typedef struct {
	pid_t aplay_pid;
	int aplay_input[2];
} aplay_info;



static void
start_aplay (MixerOutput *o)
{
	aplay_info *i;
	int aplay_input[2];
	
	if (!o)
		return;
	if (!o->data)
		return;
	i = (aplay_info *) o->data;
  
	/* Fork the aplay process */

	i->aplay_pid = fork ();
	if (i->aplay_pid) {
		close (i->aplay_input[0]);
	} else {
		char sample_rate_arg[128];
		char channels_arg[128];

		sprintf (sample_rate_arg, "%d", o->rate);
		sprintf (channels_arg, "%d", o->channels);
		close (0);
		dup (i->aplay_input[0]);

		close (i->aplay_input[0]);
		close (i->aplay_input[1]);

		execlp ("aplay", "aplay", "-t", "raw", "-f", "s16", "-r", sample_rate_arg, "-c", channels_arg, NULL);
	}
}



static void
stop_aplay (MixerOutput *o)
{
	aplay_info *i;

	if (!o)
		return;
	if (!o->data)
		return;

	i = (aplay_info *) o->data;
	kill (i->aplay_pid, SIGTERM);
	waitpid (i->aplay_pid, NULL, 0);
}



static void
aplay_mixer_output_free_data (MixerOutput *o)
{
	aplay_info *i;

	assert (o != NULL);
	assert (o->data != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "aplay_mixer_output_free_data called\n");
	i = (aplay_info *) o->data;

	stop_aplay (o);

	/* Close the pipes */

	close (i->aplay_input[0]);
	close (i->aplay_input[1]);

	free (o->data);
}



static void
aplay_mixer_output_post_data (MixerOutput *o)
{
	aplay_info *i;
  
	assert (o != NULL);
	i = (aplay_info *) o->data;

	write (i->aplay_input[1], o->buffer,
	       o->buffer_size*sizeof(short)*o->channels);
}



MixerOutput *
aplay_mixer_output_new (const char *name,
			const int rate,
			const int channels,
			const int latency)
{
	MixerOutput *o;
	aplay_info *i;
  
	i = malloc (sizeof (aplay_info));
	if (!i)
		return NULL;

	/* Create pipes */

	pipe (i->aplay_input);
	fcntl (i->aplay_input[1], F_SETFL, O_NONBLOCK);
	fcntl (i->aplay_input[1], F_SETFD, FD_CLOEXEC);

	o = malloc (sizeof (MixerOutput));
	if (!o) {
		free (i);
		return NULL;
	}
  
	o->name = strdup (name);
	o->rate = rate;
	o->channels = channels;
	o->data = (void *) i;
	o->enabled = 1;
  
	/* Overrideable methods */

	o->free_data = aplay_mixer_output_free_data;
	o->post_data = aplay_mixer_output_post_data;

	mixer_output_alloc_buffer (o, latency);

	start_aplay (o);

	return o;
}



