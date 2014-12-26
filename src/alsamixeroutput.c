/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <alsa/asoundlib.h>
#include <errno.h>
#include <string.h>
#include "debug.h"
#include "alsamixeroutput.h"



typedef struct {
	snd_pcm_t *pcm;
} alsa_info;



static void
alsa_mixer_output_free_data (MixerOutput *o)
{
	alsa_info *i;

	assert (o != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "alsa_mixer_output_free_data called for %s\n",
		      o->name);
	assert (o->data != NULL);
	i = (alsa_info *) o->data;
	snd_pcm_close (i->pcm);
	free (o->data);
}



static void
alsa_mixer_output_post_data (MixerOutput *o)
{
	alsa_info *i;
	int rc;

	assert (o != NULL);
	i = (alsa_info *) o->data;

	rc = snd_pcm_writei (i->pcm, o->buffer, o->buffer_size);
	if (rc == -EPIPE) {
		snd_pcm_recover (i->pcm, rc, 1);
		rc = snd_pcm_writei (i->pcm, o->buffer, o->buffer_size);
	}
	if (rc <= 0) {
		printf ("write result %d\n", rc);
	}
}



MixerOutput *
alsa_mixer_output_new (const char *sc_name,
		      const char *name,
		      int rate,
		      int channels,
		      int latency)
{
	MixerOutput *o;
	alsa_info *i;
	int rc;

	assert (name != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "alsa_mixer_output_new (\"%s\", %d, %d, %d)\n",
		      name, rate, channels, latency);
	assert (rate > 0);
	assert ((channels == 1) || (channels == 2));
	assert (latency > 0);
	i = malloc (sizeof (alsa_info));
	assert (i != NULL);

	/* Open the sound device */

	rc = snd_pcm_open (&(i->pcm), sc_name, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if (rc != 0) {
		printf ("snd_pcm_open failed\n");
		exit (EXIT_FAILURE);
	}
	snd_pcm_nonblock (i->pcm, 1);
	rc = snd_pcm_set_params (i->pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, channels, rate, 0, latency * 1000000 / rate);
	if (rc != 0) {
		printf("snd_pcm_set_params failed\n");
		exit (EXIT_FAILURE);
	}
	o = malloc (sizeof (MixerOutput));
	assert (o != NULL);
	o->name = strdup (name);
	o->rate = rate;
	o->channels = channels;
	o->data = (void *) i;
	o->enabled = 1;

	/* Overrideable methods */

	o->free_data = alsa_mixer_output_free_data;
	o->post_data = alsa_mixer_output_post_data;

	mixer_output_alloc_buffer (o, latency);
	return o;
}
