/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include "mixerchannel.h"
#include "soundcard.h"



typedef struct {
	int fd;
} oss_info;



static int
oss_mixer_channel_get_data (MixerChannel *ch)
{
	oss_info *i;
	int remainder;
	short *tmp;
	int rv;
  
	if (!ch)
		return -1;
	tmp = ch->input;
	remainder = ch->chunk_size*ch->channels;
	i = (oss_info *) ch->data;
	while (remainder > 0) {
		rv = read (i->fd, tmp, remainder*sizeof(short));
		if (rv <= 0)
			break;
		remainder -= rv/sizeof(short);
		tmp += rv/sizeof(short);
	}
	return ch->chunk_size-(remainder/ch->channels);
}



static void
oss_mixer_channel_free_data (MixerChannel *ch)
{
	oss_info *i;
	int tmp;
  
	if (!ch)
		return;
	i = (oss_info *) ch->data;
	if (!i)
		return;
	close (i->fd);
	free (i);
}



MixerChannel *
oss_mixer_channel_new (const char *sc_name,
		       const char *name,
		       int rate,
		       int channels,
		       int latency)
{
	MixerChannel *ch;
	oss_info *i;
	int tmp;
	int fragment_size;
	
	i = malloc (sizeof (oss_info));
	if (!i)
		return NULL;

	/* Open the sound device */

	i->fd = soundcard_setup (sc_name, rate, channels, soundcard_write, latency);
	if (i->fd < 0) {
		free (i);
		return NULL;
	}
	ch = mixer_channel_new (rate, channels, latency);
	if (!ch) {
		close (i->fd);
		free (i);
		return NULL;
	}
  
	ch->data = (void *) i;
	ch->name = strdup (name);
	ch->location = NULL;
  
	/* Overrideable methods */

	ch->get_data = oss_mixer_channel_get_data;
	ch->free_data = oss_mixer_channel_free_data;

	return ch;
}
