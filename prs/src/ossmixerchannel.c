/*
 * PRS - Personal Radio Station
 *
 * Copyright 2003 Marc Mulcahy
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * ossmixerchannel.c: Implementation of a mixer channel which gets its audio
 * from an oss soundcard.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
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
		return;
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
	if (!soundcard_get_duplex ()) {
		close (i->fd);
		soundcard_set_fd (-1);
	}
	free (i);
}



MixerChannel *
oss_mixer_channel_new (const char *name,
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

	i->fd = soundcard_get_fd ();
	if (i->fd < 0)
		i->fd = soundcard_setup (rate, channels, latency);
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
