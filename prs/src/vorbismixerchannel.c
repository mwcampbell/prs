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
 * vorbismixerchannel.c: Mixer channel which gets audio frm an Ogg Vorbis file.
 *
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <vorbis/vorbisfile.h>
#include "debug.h"
#include "mixerchannel.h"



typedef struct {
	OggVorbis_File *vf;
	int section;
	FILE *fp;
} vorbis_file_info;



static int
vorbis_mixer_channel_get_data (MixerChannel *ch)
{
	vorbis_file_info *i;
	int remainder;
	short *tmp;
	int rv;
	
	assert (ch != NULL);
	tmp = ch->input;
	remainder = ch->chunk_size*ch->channels;
	i = (vorbis_file_info *) ch->data;
	assert (ch->data != NULL);
	while (remainder > 0) {
		rv = ov_read (i->vf, (char *) tmp, remainder * sizeof (short),
			      0, sizeof (short), 1, &(i->section));
		if (rv <= 0)
			break;
		remainder -= rv / sizeof (short);
		tmp += rv / sizeof (short);
	}
	return ch->chunk_size-(remainder/ch->channels);
}



static void
vorbis_mixer_channel_free_data (MixerChannel *ch)
{
	vorbis_file_info *i;

	assert (ch != NULL);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "vorbis_mixer_channel_free_data called for %s\n",
		      ch->name);
	i = (vorbis_file_info *) ch->data;
	assert (i != NULL);
	ov_clear (i->vf);
	free (i->vf);
	free (i);
}



MixerChannel *
vorbis_mixer_channel_new (const char *name,
			  const char *location,
			  const int mixer_latency)
{
	MixerChannel *ch;
	vorbis_file_info *i;
	vorbis_info *vi;
  
	assert (name != NULL);
	assert (location != NULL);
	assert (mixer_latency > 0);
	debug_printf (DEBUG_FLAGS_MIXER,
		      "vorbis_mixer_channel_new (\"%s\", \"%s\", %d)\n",
		      name, location, mixer_latency);
	i = malloc (sizeof (vorbis_file_info));
	assert (i != NULL);

	/* Allocate the OggVorbis_file structure */

	i->vf = malloc (sizeof (OggVorbis_File));
	assert (i->vf != NULL);
  
	/* Open the file */

	i->fp = fopen (location, "r+b");
	if (!(i->fp)) {
		free (i->vf);
		free (i);
		debug_printf (DEBUG_FLAGS_MIXER,
			      "vorbis_mixer_channel_new: can't open %s: %s\n",
			      location, strerror (errno));
		return NULL;
	}
  
	ov_open (i->fp, i->vf, NULL, 0);

	/* Get sample rate information from file */

	vi = ov_info (i->vf, -1);
	assert (vi != NULL);
	ch = mixer_channel_new (vi->rate,
				vi->channels,
				mixer_latency);

	ch->data = (void *) i;
	ch->name = strdup (name);
	ch->location = strdup (location);
  
	/* Set overrideable methods */

	ch->get_data = vorbis_mixer_channel_get_data;
	ch->free_data = vorbis_mixer_channel_free_data;

	mixer_channel_start_reader (ch);
	return ch;
}
